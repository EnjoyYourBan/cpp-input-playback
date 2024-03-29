
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <algorithm>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

void captureInput(std::vector<int> &inputs, std::vector<std::chrono::system_clock::time_point> &timepoints, Display *dis) {
    // push initial time to compare inputs
    timepoints.push_back(std::chrono::system_clock::now());
    XEvent ev;
    int key;

    //TODO: pass this to a function, add glowy keys? 
    Window root = DefaultRootWindow(dis);
    Window window = XCreateSimpleWindow(dis, root, 0, 0, 800, 400, 1, 0, 0);
    XSelectInput(dis, window, KeyPressMask | KeyReleaseMask);

    XMapWindow(dis, window); // draw the window 


    for(;;) {
        XNextEvent(dis, &ev);
        if (ev.type != KeyPress && ev.type != KeyRelease)
            continue;

        key = static_cast<int>(XLookupKeysym(&ev.xkey, 0));
        std::cout << "debug: " << key << std::endl;

        if (key == XK_Escape) break;
        inputs.push_back(ev.type == KeyPress ? key : -key);
        timepoints.push_back(std::chrono::system_clock::now());

    } 

    XUnmapWindow(dis, window);
    
}

void playbackInput(const std::vector<int> &inputs, std::vector<int> &timepoints, Display *dis, bool loop) {
    int timer_old = 0;
    int key;
    if (loop)
        std::cout << "WARNING! Loop is active, press CTRL+C or close program to cancel.";

    for(;;) {
        for (size_t i = 0; i < inputs.size(); ++i) {
            key = XKeysymToKeycode(dis, std::abs(inputs[i]));
            std::this_thread::sleep_for(std::chrono::milliseconds(timepoints[i] - timer_old));

            // Windows should use SendInput() when i care to add support
            XTestFakeKeyEvent(dis, key, (inputs[i] > 0), 0); // UNCLICK the key because this works for some reason
            XFlush(dis);

            timer_old = timepoints[i];
        }

        if (!loop) return;
    }
}

void makeSaveFile(std::string filePath, 
                const std::vector<int> &inputs, 
                const std::vector<std::chrono::system_clock::time_point>& timepoints) {

    std::ofstream file(filePath);

    if (!file.is_open()) {
        std::cout << "Unable to open file at file path." << std::endl;
        return;
    }

    auto timer_start = std::chrono::duration_cast<std::chrono::milliseconds>(timepoints[0].time_since_epoch());
    int timer_startms = static_cast<int>(timer_start.count());

    for (size_t i = 0; i < inputs.size(); ++i) {
        timer_start = std::chrono::duration_cast<std::chrono::milliseconds>(timepoints[i+1].time_since_epoch());
        file << inputs[i] << "::" << static_cast<int>(timer_start.count()) - timer_startms << std::endl;
    }

    file.close();
}

void readPlaybackFile(std::string path, std::vector<int> &inputs, std::vector<int> &timepoints) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "Failed to read file at path: "  << path << std::endl;
        return;
    }
    std::string line;
    int keycode;
    int timepoint;
    int linepos;

    while (std::getline(file, line)) {
        if (line.empty()) break;
        linepos = line.find("::");
        try {
            keycode = stoi(line.substr(0, linepos));
            timepoint = stoi(line.substr(linepos+2, line.length()));
        } catch (const std::invalid_argument &e) {
            std::cerr << "Invalid line ->" << line << std::endl;
            continue;
        }

        timepoints.push_back(timepoint);
        inputs.push_back(keycode);
    }
}

int main(int argc, char *argv[]) {
    std::vector<std::string> args(argv, argv+argc); 
    std::vector<int> inputs;
    Display *dis;
    std::string dir;
    dis = XOpenDisplay(NULL);
    if (!dis) return 1;



    if (argc > 1) {
        std::vector<int> timestamps;
        bool loop = (std::find(args.begin(), args.end(), "-loop") != args.end());
        readPlaybackFile(argv[1], inputs, timestamps);
        playbackInput(inputs, timestamps, dis, loop);
        XCloseDisplay(dis);
        return 0;
    }

    std::vector<std::chrono::system_clock::time_point> timepoints;

    std::cout << "(ESC -> EXIT)" << std::endl;
    captureInput(inputs, timepoints, dis);
    XCloseDisplay(dis);
    std::cout << "Save path: ";
    std::cin >> dir;

    makeSaveFile(dir, inputs, timepoints);

    return 0;

}

#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
#include <thread>
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>

void captureInput(std::vector<KeySym>& inputs, std::vector<std::chrono::system_clock::time_point>& timepoints, Display *dis) {
    // push initial time to compare inputs
    timepoints.push_back(std::chrono::system_clock::now());
    XEvent ev;
    KeySym key;

    //TODO: pass this to a function, add glowy keys? 
    Window root = DefaultRootWindow(dis);
    Window window = XCreateSimpleWindow(dis, root, 0, 0, 800, 400, 1, 0, 0);
    XSelectInput(dis, window, KeyPressMask);

    XMapWindow(dis, window); // draw the window 


    do {
        XNextEvent(dis, &ev);
        if (ev.type != KeyPress)
            continue;

        key = XLookupKeysym(&ev.xkey, 0);
        std::cout << "debug: " << key << std::endl;

        inputs.push_back(key);
        timepoints.push_back(std::chrono::system_clock::now());

    } while (key != XK_Escape); // loop exit on escape

    XUnmapWindow(dis, window);
    
}

void playbackInput(const std::vector<uint8_t> &inputs, std::vector<int> &timepoints, Display *dis) {
    int timer_old = 0;
    KeyCode key;
    XEvent ev;

    for (size_t i = 0; i < inputs.size(); ++i) {
        key = XKeysymToKeycode(dis, inputs[i]);
        std::this_thread::sleep_for(std::chrono::milliseconds(timepoints[i] - timer_old));
        // Windows should use SendInput() when i care to add support
        XTestFakeKeyEvent(dis, key, false, 0); // UNCLICK the key because this works for some reason
        XFlush(dis);
        XTestFakeKeyEvent(dis, key, true, 0);
        XFlush(dis);
        XTestFakeKeyEvent(dis, key, false, 0);
        XFlush(dis);

        std::cout << (key) << std::endl;
        timer_old = timepoints[i];
    }
}

void makeSaveFile(std::string filePath, 
                const std::vector<KeySym> &inputs, 
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

void readPlaybackFile(std::string path, std::vector<uint8_t> &inputs, std::vector<int> &timepoints) {
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

    Display *dis;
    std::string dir;
    dis = XOpenDisplay(NULL);
    if (!dis) return 1;

    if (argc > 1) {
        std::vector<uint8_t> inputs;
        std::vector<int> timestamps;
        readPlaybackFile(argv[1], inputs, timestamps);
        playbackInput(inputs, timestamps, dis);
        XCloseDisplay(dis);
        return 0;
    }


    std::vector<KeySym> inputs;
    std::vector<std::chrono::system_clock::time_point> timepoints;

    std::cout << "(ESC -> EXIT)" << std::endl;
    captureInput(inputs, timepoints, dis);
    XCloseDisplay(dis);
    std::cout << "Save path: ";
    std::cin >> dir;

    makeSaveFile(dir, inputs, timepoints);

    return 0;

}
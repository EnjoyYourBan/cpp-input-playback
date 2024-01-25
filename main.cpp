
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
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

void playbackInput(const std::vector<KeySym>& inputs, std::vector<std::chrono::system_clock::time_point>& timepoints, Display *dis) {
    auto timer_start = std::chrono::duration_cast<std::chrono::milliseconds>(timepoints[0].time_since_epoch());
    int timer_startms = static_cast<int>(timer_start.count());
    KeyCode key;
        XEvent ev;
        for (size_t i = 0; i < inputs.size(); ++i) {
            timer_start = std::chrono::duration_cast<std::chrono::milliseconds>(timepoints[i+1].time_since_epoch());
            key = XKeysymToKeycode(dis, inputs[i]);

            std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(timer_start.count()) - timer_startms));

            timer_startms = static_cast<int>(timer_start.count());
            // Windows should use SendInput() when i care to add support
            XTestFakeKeyEvent(dis, key, false, 0); // UNCLICK the key because this works for some reason
            XFlush(dis);
            XTestFakeKeyEvent(dis, key, true, 0);
            XFlush(dis);
            XTestFakeKeyEvent(dis, key, false, 0);
            XFlush(dis);

            std::cout << (key) << std::endl;
        }
}

int main(int argc, char *argv[]) {
    std::vector<KeySym> inputs;
    std::vector<std::chrono::system_clock::time_point> timepoints;
    Display *dis;
    dis = XOpenDisplay(NULL);

    if (!dis) return 1;
    std::cout << "(ESC -> EXIT)" << std::endl;
    captureInput(inputs, timepoints, dis);

    std::cout << "\ndebug output:" << std::endl;
    
    playbackInput(inputs, timepoints, dis);

    XCloseDisplay(dis);
    return 0;
}
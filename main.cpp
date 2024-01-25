#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

/** not gonna even pretend to understand what im doing*/
void instantInput() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void oldInput() {
    struct termios t;
    tcgetattr(STDIN_FILENO, &t);
    t.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
}

void captureInput(std::vector<char>& inputs, std::vector<std::chrono::system_clock::time_point>& timepoints) {
    instantInput();
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL, 0) | O_NONBLOCK);


    // push initial time to compare inputs
    timepoints.push_back(std::chrono::system_clock::now());

    char key;
    do {
        int result = read(STDIN_FILENO, &key, 1);
        if (result == 0 || result == -1)
            continue;

        inputs.push_back(key);
        timepoints.push_back(std::chrono::system_clock::now());

    } while (key != 27); // loop exit on escape

    oldInput();
}


void playbackInput(const std::vector<char>& inputs, std::vector<std::chrono::system_clock::time_point>& timepoints) {
    auto timer_start = std::chrono::duration_cast<std::chrono::milliseconds>(timepoints[0].time_since_epoch());
    int timer_startms = static_cast<int>(timer_start.count());
    char key;

    for (size_t i = 0; i < inputs.size(); ++i) {
        timer_start = std::chrono::duration_cast<std::chrono::milliseconds>(timepoints[i+1].time_since_epoch());
        key = inputs[i];

        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(timer_start.count()) - timer_startms));

        timer_startms = static_cast<int>(timer_start.count());
        std::cout << (key) << std::endl;
    }

}

int main(int argc, char *argv[]) {
    std::vector<char> inputs;
    std::vector<std::chrono::system_clock::time_point> timepoints;

    std::cout << "(ESC -> EXIT)" << std::endl;
    captureInput(inputs, timepoints);

    std::cout << "\ndebug output:" << std::endl;
    playbackInput(inputs, timepoints);

    return 0;
}
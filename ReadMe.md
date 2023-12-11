# My Header-Only Thread Pool Library

## Installation
Run `sudo make install` to install the library to `/usr/local/include/`.

If you want to install it elsewhere, you can run the command as `INCLUDEDIR=/some/other/install/directory sudo make install`.

## Uninstallation
Run `sudo make uninstall` to uninstall the library from the `$INCLUDEDIR` environment variable (defaults to `/usr/local/include/`).

If you have forgotten where you installed it for some reason, you can see where it's located with the c-preprocessor (via `cpp` or `g++ -E`) and just remove it yourself. It's just header file.

## Usage
An example usage is below. Please make sure to run your compilation command with the `-pthread` flag.
```cpp
#include <iostream>
#include <string>
#include <unistd.h>
#include <ThreadPool> // Include my library

int main(void){
    using uint = unsigned int;

    uint nproc = std::thread::hardware_concurrency();

    ThreadPool p(nproc); // Initialize the thread pool with nproc threads
    std::vector<std::future<std::string>> futures; futures.resize(nproc); // Create a vector of futures

    char c = 'A';
    for(auto& f : futures){
        // Enqueue the lambda task with the ThreadPool, storing the future so we can get our result later (and to prevent blocking).
        f = p.enqueue([](const char c) {
            sleep(2); // Sleep for 2 seconds. The *entire program* should only sleep for 2-3 seconds, due to the multithreading.
            return std::string(20, c);
        }, c++);
    }

    // Print out our results.
    for(auto& f : futures){
        std::cout << f.get() << "\n";
    }

    std::cout << std::flush;
    return 0;
}

```

### Why
I needed a thread pool and wanted to learn more about C++ threading and templating.
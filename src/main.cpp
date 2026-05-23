#include "ui/UserInterface.h"
#include <iostream>

int main() {
    // Instantiate orchestrator and spin up UI processing loop

    const std::string datafilePath = "books_dataset.txt";
    mislib::UserInterface ui(datafilePath);
    ui.run();


    return 0;
}

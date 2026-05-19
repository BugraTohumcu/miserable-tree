#include "ui/UserInterface.h"

int main() {
    // Instantiate orchestrator and spin up UI processing loop
    mislib::UserInterface ui;
    ui.run();

    return 0;
}

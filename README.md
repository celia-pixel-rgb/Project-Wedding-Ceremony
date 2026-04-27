# Project-Wedding-Ceremony

Simulation of a real-world wedding management system

DOWNLOAD AND INSTALLATION OF THE GTK LIBRARY

We will guide you through all the steps yoou will have to undertake inorder to properly use our program, including the necessary libraries and commands to run it efficiently.

First, you have to download the installer at msys2.org, selecting the first version.

Following, at the end of the download process, ensure that you store the installer in a folder you will easily remember e.g wedding management ceremony

Again, open the "MSYS2 MSYS" terminal and run the command "pacman -Syu".

Still, run the command "$ pacman -S mingw-w64-ucrt-x86_64-gcc" into the "MSYS2 UCRT64" terminal downloaded alongside the GTK library, and validate all the options till the end of the various installations.

To continue, install GTK (GUI library),  still in the "MSYS2 MINGW64" terminal by typing in the command "pacman -S mingw-w64-x86_64-gtk4", and then install build tools by typing-in the command "pacman -S mingw-w64-x86_64-pkgconfig mingw-w64-x86_64-make" in the same terminal.

Next, in order to ensure that the gcc compiler was well installed, run the command "gcc --version" into the "MSYS2 MINGW64" terminal. You should see the version afterwards.

Before running the code, you must make sure to store it in the same folder as the one which contains your GTK libraries.

In order to run the code, you must open the "MSYS URT64" terminal and type the command "gcc file.c -o file.exe $(pkg-config --cflags --libs gtk4)"

If the code was successfully run, a ".exe" file appears just below your c-code in the folder.

Open it, this is your executable file !

- The csv file is generated automatically too!

NB -Each module was made separate, inorder to see the modules interacting with each other, run the combinedmodules.c in devc++ or the gtkcombinedmodules.c in the MSYS2UCR64 terminal.

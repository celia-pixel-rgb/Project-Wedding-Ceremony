# Project-Wedding-Ceremony

Simulation of a real-world wedding management system

In order to run the Graphical User Inteface of our application, you have to install the GTK library, found in msys2

DOWNLOAD AND INSTALLATION OF THE GTK LIBRARY

First, you have to download the installer at msys2.org, selecting the first version.

<img width="469" height="179" alt="image" src="https://github.com/user-attachments/assets/3a99db95-72a1-4f5a-8e5d-84b71abe44a7" />

Following, at the end of the download process, ensure that you store the installer in a folder you will easily remember e.g wedding management ceremony

Again, open the "MSYS2 MSYS" terminal and run the command "pacman -Syu".

<img width="550" height="230" alt="image" src="https://github.com/user-attachments/assets/6755dc0f-8788-424d-b24c-05e5c4ee2e0b" />


Still, run the command "$ pacman -S mingw-w64-ucrt-x86_64-gcc" into the "MSYS2 UCRT64" terminal downloaded alongside the GTK library, and validate all the options till the end of the various installations.

<img width="1134" height="550" alt="image" src="https://github.com/user-attachments/assets/a62ad9b1-3208-4331-b781-7e99aadf435e" />


To continue, install GTK (GUI library),  still in the "MSYS2 MINGW64" terminal by typing in the command "pacman -S mingw-w64-x86_64-gtk4", and then install build tools by typing-in the command "pacman -S mingw-w64-x86_64-pkgconfig mingw-w64-x86_64-make" in the same terminal.

<img width="543" height="221" alt="image" src="https://github.com/user-attachments/assets/be68214c-8c0c-4779-8ea2-9d051e9df866" />

Next, in order to ensure that the gcc compiler was well installed, run the command "gcc --version" into the "MSYS2 MINGW64" terminal. You should see the version afterwards.

<img width="545" height="264" alt="image" src="https://github.com/user-attachments/assets/affe0e3a-ab56-4b07-97f9-6dad0e8e9a9d" />

Before running the code, you must make sure to store it in the same folder as the one which contains your GTK libraries.


In order to run the code, you must open the "MSYS URT64" terminal and type the command "gcc file.c -o file.exe $(pkg-config --cflags --libs gtk4)"

If the code was successfully run, a ".exe" file appears just below your c-code in the folder.

<img width="501" height="74" alt="image" src="https://github.com/user-attachments/assets/56d32b78-d4c8-4440-9f1f-38f4adec48b6" />

Open it, this is your executable file !

- The csv file is generated automatically too!
  
  <img width="523" height="78" alt="image" src="https://github.com/user-attachments/assets/8d8e5521-678b-4567-997c-0ca0eb56fd2d" />

<img width="1077" height="256" alt="image" src="https://github.com/user-attachments/assets/b28734fa-b833-4dd3-bd41-bb8e89f6bc3f" />

NB -Each module was made separate, inorder to see the modules interacting with each other, run the combinedmodules.c in devc++ or the gtkcombinedmodules.c in the MSYS2UCR64 terminal.

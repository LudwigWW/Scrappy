# Scrappy

Source code for the CHI2021 paper 
**Scrappy: Using Scrap Material as Infill to Make Fabrication More Sustainable**
DOI: https://doi.org/10.1145/3411764.3445187

# Server side installation

Using a Linux system for the server could simplify some installation steps.

1. Install libigl [here](https://libigl.github.io/).
2. Add the `libigl_for_matryoshka` extension to your libigl installation (Copy the folder contents into the libigl base folder).
3. Install Matlab (R2017a).
4. Adjust the `Matlab_ROOT_DIR` variable in `FindLIBIGL.cmake` with the path to matlab.
```sh
In: server/matryoshka/cmake/FindLIBIGL.cmake

set(Matlab_ROOT_DIR "/path/to/matlab")
```
5. Adjust the `contFilePath` variable in `matryoshkaModular.cpp` with the path to `scrapLibrary/contFiles/`.
```sh
In: server/matryoshka/matryoshkaModular.cpp, line 236

contFilePath = "/path/to/scrappyLibrary/contFiles/" [...]
```
6. Build the matryoshka project.
```sh
cd server/matryoshka
mkdir Release
cd Release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8
```
7. Install venv.
8. Start the django server.
```sh
cd server/server
source bin/activate
cd django/scrappy
python manage.py runserver 0.0.0.0:47555
```
9. Adjust IP and run a server for a client IP.
```sh
In: server/server/server.py, lines 15 and 19
Adjust IP address to match clients

cd server/server
source bin/activate
python server.py
```
# Client side installation

1. Install Fusion360.
2. Add Scrappy add-in to Fusion360.
```sh
Fusion360 -> Tools -> ADD-INNS --> Add-Inns --> Click the + sign to add existing add-in
Choose folder: client/Fusion360/ScrappyAdd-In
```
3. Adjust IP to match server IP in `ScrappyAdd-In.py`.
```sh
In: client/Fusion360/ScrappyAdd-In/ScrappyAdd-In.py, line 56
Adjust IP to matcher server IP
```
4. In Fusion360, run the add-in.
5. Show the scrapLibrary panel from the Tools panel to start using Scrappy.
6. Install node.js.
7. Start the kiri:moto slicer .
```
cd client/grid-apps/
npm i
npm start
```
8. Open [Kiri:Moto](http://localhost:8080/kiri) to use the slicer.

# Network

1. Ensure ports 46337-46998 and 47101-47557 are open.
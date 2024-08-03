# project description
this is a simplex real-time audio steamer made in c and runs on linux os leveraging the portaudio for audio recording and playing functionality between two devices .

# version information
1)initial_release -> this is the most basic form of the project with a simple UDP connection through which audio is streamed from sender to receiver in real-time .

2)update_1.1a -> this update adds a checksum and ACK system to the UDP connection to provide a more reliable connection with minimal increase in latency . 

3)update_1.1b -> this update utilizes a TCP connection instead of a UDP connection and provides further more reliable connection ,with the drawback being it needs a stable network connection to prevent latency issues .

4)update_2 -> this update utilizes a TCP connection similar to update_1.1b but introduces a relay_server between the sender and receiver which can be further deployed on a cloud instance to enable audio streaming through cloud . 

# how to run (upto version 1.1) ?
1 a) first install port-audio in your system if its not already installed ,enter the below commnand in your linux terminal for Debian-based linux distributions ->

sudo apt-get install -y portaudio19-dev libasound-dev build-essential

1 b) first install port-audio in your system if its not already installed ,enter the below commnand in your linux terminal for Red Hat-based linux distributions ->

sudo dnf install -y portaudio-devel alsa-lib-devel gcc

3) copy sender and receiver c files onto any two pcs , and change the ip-address in the sender file to that of receiver pc ip .

4) compile programs on both pcs using below commands on sender and receiver side terminals in their directories respectively using below commands ->
   
gcc -o sender sender.c -lportaudio

gcc -o receiver receiver.c -lportaudio

5) run both programs on sender and receiver side terminals in their directories respectively using below commands ->
   
./sender

./receiver

# how to run (for version 2) ?
1 a) first install port-audio in your system if its not already installed ,enter the below commnand in your linux terminal for Debian-based linux distributions ->

sudo apt-get install -y portaudio19-dev libasound-dev build-essential

1 b) first install port-audio in your system if its not already installed ,enter the below commnand in your linux terminal for Red Hat-based linux distributions ->

sudo dnf install -y portaudio-devel alsa-lib-devel gcc

3) copy sender , receiver and relay_server c files onto any three pcs , and change the ip-address in the sender file to that of receiver pc ip .

4) compile programs on all three pcs using below commands on sender , receiver , relay_server side terminals in their directories respectively using below commands ->
   
gcc -o sender sender.c -lportaudio

gcc -o receiver receiver.c -lportaudio

gcc -o relay_server relay_server.c -lpthread

5) run the program on the relay server terminal after compiling it in its directory using below command ->

./relay_server

5) run the program on the sender side terminal after compiling it in its directory using below command ->
   
./sender

6) after receiving confirmation message in the relay_server terminal that the sender is connected run the program on the receiver side terminal after compiling it in its directory using below command ->

./receiver

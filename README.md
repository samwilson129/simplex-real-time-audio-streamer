# project description
this is a simplex real-time audio steamer made in c and runs on linux os leveraging the portaudio for audio record and playing functionality between two devices .

# version information
1)initial_release -> this is the most basic form of the project with a simple UDP connection through which audio is streamed from sender to receiver in real-time .

2)update_1.1a -> this update adds a checksum and ACK system to the UDP connection to provide a more reliable connection with minimal increase in latency

3)update_1.1b -> this update utilizes a TCP connection instead of a UDP connection and provides further more reliable connection ,with the drawback being it needs a stable network connection to prevent latency issues .

# how to run ?
1) first install port-audio in your system if its not already installed ,enter the below commnand in your linux terminal ->

sudo apt-get install portaudio19-dev

3) copy sender and receiver c files onto any two pcs , and change the ip-address in the sender file to that of receiver pc ip .

4) compile programs on both pcs using below commands on sender and receiver side terminals in their directories respectively using below commands ->
   
gcc -o sender sender.c -lportaudio

gcc -o receiver receiver.c -lportaudio

5) run both programs on sender and receiver side terminals in their directories respectively using below commands ->
   
./sender

./receiver




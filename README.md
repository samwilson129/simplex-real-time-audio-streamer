# project description
this is a simplex real-time audio steamer made in c and runs on linux os leveraging the portaudio for audio record and playing functionality and using udp as the networking means for the same .

# how to run (for initial_release) ?
1) first install port-audio in your system if its not already installed ,enter the below commnand in your linux terminal ->

sudo apt-get install portaudio19-dev

3) copy sender and receiver c files onto any two pcs , and change the ip-address in the sender file to that of receiver pc ip .

4) compile programs on both pcs using below commands on sender and receiver side terminals in their directories respectively using below commands ->
   
gcc -o sender sender.c -lportaudio

gcc -o receiver receiver.c -lportaudio

5) run both programs on sender and receiver side terminals in their directories respectively using below commands ->
   
./sender

./receiver




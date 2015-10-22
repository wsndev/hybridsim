./waf --run "scratch/lr-wpan-data --inputFile=send_64.txt --outputFile=tx.txt"
time (
sudo ./lrwpan-file r /dev/ttyUSB0 rx.txt &
sudo ./lrwpan-file s /dev/ttyUSB1 tx.txt &
wait)
./waf --run "scratch/lr-wpan-data --inputFile=rx.txt --outputFile=receive.txt"

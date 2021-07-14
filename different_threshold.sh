make
echo "" > result.txt
echo "" > running.log

echo "===== setting: -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.5" >> result.txt
./sasimi-vecbee -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.5 1>>running.log 2>>result.txt
echo "" >> result.txt

echo "===== setting: -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.3" >> result.txt
./sasimi-vecbee -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.3 1>>running.log 2>>result.txt
echo "" >> result.txt

echo "===== setting: -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.2" >> result.txt
./sasimi-vecbee -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.2 1>>running.log 2>>result.txt
echo "" >> result.txt

echo "===== setting: -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.125" >> result.txt
./sasimi-vecbee -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.125 1>>running.log 2>>result.txt
echo "" >> result.txt

echo "===== setting: -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.05" >> result.txt
./sasimi-vecbee -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.05 1>>running.log 2>>result.txt
echo "" >> result.txt


make
echo "" > result.txt
echo "" > running.log

echo "===== setting: -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.5" >> result.txt
./sasimi-vecbee -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.05 1>>running.log 2>>result.txt
echo "" >> result.txt

echo "===== setting: -m maxed -s area -a noEncode -c 2.0 -t 40 -p 1 -d 0.5" >> result.txt
./sasimi-vecbee -m maxed -s area -a noEncode -c 2.0 -t 40 -p 1 -d 0.05 1>>running.log 2>>result.txt
echo "" >> result.txt

echo "===== setting: -m maxed -s area -a noEncode -c 2.0 -t 60 -p 1 -d 0.5" >> result.txt
./sasimi-vecbee -m maxed -s area -a noEncode -c 2.0 -t 60 -p 1 -d 0.05 1>>running.log 2>>result.txt
echo "" >> result.txt

# echo "===== setting: -m maxed -s area -a noEncode -c 1.0 -t 20 -p 1 -d 0.5" >> result.txt
# ./sasimi-vecbee -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.05 1>>running.log 2>>result.txt
# echo "" >> result.txt

# echo "===== setting: -m maxed -s area -a noEncode -c 0.5 -t 20 -p 1 -d 0.5" >> result.txt
# ./sasimi-vecbee -m maxed -s area -a noEncode -c 2.0 -t 20 -p 1 -d 0.05 1>>running.log 2>>result.txt
# echo "" >> result.txt

# echo "===== setting: -m maxed -s area -a noEncode -c 2.0 -t 20 -p 0 -d 0.5" >> result.txt
# ./sasimi-vecbee -m maxed -s area -a noEncode -c 2.0 -t 20 -p 0 -d 0.05 1>>running.log 2>>result.txt
# echo "" >> result.txt

# echo "===== setting: -m er -s area -a noEncode -c 2.0 -t 20 -p 1" >> result.txt
# ./sasimi-vecbee -m er -s area -a noEncode -c 2.0 -t 20 -p 1 1>>running.log 2>>result.txt
# echo "" >> result.txt

# echo "===== setting: -m nmed -s area -a noEncode -c 2.0 -t 20 -p 1" >> result.txt
# ./sasimi-vecbee -m nmed -s area -a noEncode -c 2.0 -t 20 -p 1 1>>running.log 2>>result.txt
# echo "" >> result.txt

# echo "===== setting: -m maxed -s area -a noEncode -c 2.0 -t 20 -p 0" >> result.txt
# ./sasimi-vecbee -m maxed -s area -a noEncode -c 2.0 -t 20 -p 0 1>>running.log 2>>result.txt
# echo "" >> result.txt
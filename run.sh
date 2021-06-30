make
echo "" > result.txt
echo "" > running.log
echo "===== setting: -m maxed -s area -a areaEncodeApprox" >> result.txt
./sasimi-vecbee -m maxed -s area -a areaEncodeApprox -c 2.0 -t 60 1>>running.log 2>>result.txt
echo "" >> result.txt
echo "===== setting: -m maxed -s area -a areaEncodeApprox" >> result.txt
./sasimi-vecbee -m maxed -s area -a areaEncodeApprox -c 1.0 -t 60 1>>running.log 2>>result.txt
echo "" >> result.txt
echo "===== setting: -m maxed -s area -a areaEncodeApprox" >> result.txt
./sasimi-vecbee -m maxed -s area -a areaEncodeApprox -c 0.5 -t 60 1>>running.log 2>>result.txt
echo "" >> result.txt

echo "===== setting: -m maxed -s area -a areaEncodeApprox" >> result.txt
./sasimi-vecbee -m maxed -s area -a areaEncodeApprox -c 2.0 -t 40 1>>running.log 2>>result.txt
echo "" >> result.txt
echo "===== setting: -m maxed -s area -a areaEncodeApprox" >> result.txt
./sasimi-vecbee -m maxed -s area -a areaEncodeApprox -c 1.0 -t 40 1>>running.log 2>>result.txt
echo "" >> result.txt
echo "===== setting: -m maxed -s area -a areaEncodeApprox" >> result.txt
./sasimi-vecbee -m maxed -s area -a areaEncodeApprox -c 0.5 -t 40 1>>running.log 2>>result.txt
echo "" >> result.txt

echo "===== setting: -m maxed -s area -a areaEncodeApprox" >> result.txt
./sasimi-vecbee -m maxed -s area -a areaEncodeApprox -c 2.0 -t 20 1>>running.log 2>>result.txt
echo "" >> result.txt
echo "===== setting: -m maxed -s area -a areaEncodeApprox" >> result.txt
./sasimi-vecbee -m maxed -s area -a areaEncodeApprox -c 1.0 -t 20 1>>running.log 2>>result.txt
echo "" >> result.txt
echo "===== setting: -m maxed -s area -a areaEncodeApprox" >> result.txt
./sasimi-vecbee -m maxed -s area -a areaEncodeApprox -c 0.5 -t 20 1>>running.log 2>>result.txt
echo "" >> result.txt

# echo "===== setting: -m nmed -s area -a areaEncodeApprox" >> result.txt
# ./sasimi-vecbee -m nmed -s area -a areaEncodeApprox 1>>running.log 2>>result.txt
# echo "" >> result.txt
# echo "===== setting: -m maxed -s area -a areaEncodeApprox" >> result.txt
# ./sasimi-vecbee -m maxed -s area -a areaEncodeApprox 1>>running.log 2>>result.txt
# # echo "" >> result.txt
# # echo "===== setting: -m area -s area -a areaEncodeApprox" >> result.txt
# # ./sasimi-vecbee -m area -s area -a areaEncodeApprox 1>>running.log 2>>result.txt

# echo "" >> result.txt
# echo "===== setting: -m er -s random -a areaEncodeApprox" >> result.txt
# ./sasimi-vecbee -m er -s random -a areaEncodeApprox 1>>running.log 2>>result.txt
# echo "" >> result.txt
# echo "===== setting: -m nmed -s random -a areaEncodeApprox" >> result.txt
# ./sasimi-vecbee -m nmed -s random -a areaEncodeApprox 1>>running.log 2>>result.txt
# echo "" >> result.txt
# echo "===== setting: -m maxed -s random -a areaEncodeApprox" >> result.txt
# ./sasimi-vecbee -m maxed -s random -a areaEncodeApprox 1>>running.log 2>>result.txt
# # echo "" >> result.txt
# # echo "===== setting: -m area -s random -a areaEncodeApprox" >> result.txt
# # ./sasimi-vecbee -m area -s random -a areaEncodeApprox 1>>running.log 2>>result.txt

# echo "" >> result.txt
# echo "===== setting: -m er -s areaError -a areaEncodeApprox" >> result.txt
# ./sasimi-vecbee -m er -s areaError -a areaEncodeApprox 1>>running.log 2>>result.txt
# echo "" >> result.txt
# echo "===== setting: -m nmed -s areaError -a areaEncodeApprox" >> result.txt
# ./sasimi-vecbee -m nmed -s areaError -a areaEncodeApprox 1>>running.log 2>>result.txt
# echo "" >> result.txt
# echo "===== setting: -m maxed -s areaError -a areaEncodeApprox" >> result.txt
# ./sasimi-vecbee -m maxed -s areaError -a areaEncodeApprox 1>>running.log 2>>result.txt
# # echo "" >> result.txt
# # echo "===== setting: -m area -s areaError -a areaEncodeApprox" >> result.txt
# # ./sasimi-vecbee -m area -s areaError -a areaEncodeApprox 1>>running.log 2>>result.txt

# echo "" >> result.txt
# echo "===== setting: -m er -s area -a noEncode" >> result.txt
# ./sasimi-vecbee -m er -s area -a noEncode 1>>running.log 2>>result.txt
# echo "" >> result.txt
# echo "===== setting: -m nmed -s area -a noEncode" >> result.txt
# ./sasimi-vecbee -m nmed -s area -a noEncode 1>>running.log 2>>result.txt
# echo "" >> result.txt
# echo "===== setting: -m maxed -s area -a noEncode" >> result.txt
# ./sasimi-vecbee -m maxed -s area -a noEncode 1>>running.log 2>>result.txt
# # ./sasimi-vecbee -m area -s area -a noEncode 1>>running.log 2>>result.txt

# echo "" >> result.txt
# echo "===== setting: -m er -s random -a noEncode" >> result.txt
# ./sasimi-vecbee -m er -s random -a noEncode 1>>running.log 2>>result.txt
# echo "" >> result.txt
# echo "===== setting: -m nmed -s random -a noEncode" >> result.txt
# ./sasimi-vecbee -m nmed -s random -a noEncode 1>>running.log 2>>result.txt
# echo "" >> result.txt
# echo "===== setting: -m maxed -s random -a noEncode" >> result.txt
# ./sasimi-vecbee -m maxed -s random -a noEncode 1>>running.log 2>>result.txt
# # ./sasimi-vecbee -m area -s random -a noEncode 1>>running.log 2>>result.txt

# echo "" >> result.txt
# echo "===== setting: -m er -s areaError -a noEncode" >> result.txt
# ./sasimi-vecbee -m er -s areaError -a noEncode 1>>running.log 2>>result.txt
# echo "" >> result.txt
# echo "===== setting: -m nmed -s areaError -a noEncode" >> result.txt
# ./sasimi-vecbee -m nmed -s areaError -a noEncode 1>>running.log 2>>result.txt
# echo "" >> result.txt
# echo "===== setting: -m maxed -s areaError -a noEncode" >> result.txt
# ./sasimi-vecbee -m maxed -s areaError -a noEncode 1>>running.log 2>>result.txt
# # echo "" >> result.txt
# # echo "===== setting: -m area -s areaError -a noEncode" >> result.txt
# # ./sasimi-vecbee -m area -s areaError -a noEncode 1>>running.log 2>>result.txt

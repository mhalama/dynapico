export PICO_SDK_PATH=~/pico/pico-sdk

. .env
cmake -B build -G Ninja -DWIFI_SSID=$WIFI_SSID -DWIFI_PASSWORD=$WIFI_PASSWORD
cmake --build build
  
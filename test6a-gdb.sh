# Adelaida
# test6a.sh
clear
make clean
make
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./mi_mkfs disco 100000\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#inicializamos el sistema de ficheros con 100.000 bloques\x1b[0m"
./mi_mkfs disco 100000
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer_sf disco\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#mostramos solo el SB\x1b[0m"
./leer_sf disco
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ ./escribir disco "123456789" 0\x1b[0m"
./escribir disco "123456789" 0
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer_sf disco\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#mostramos solo el SB\x1b[0m"
./leer_sf disco
echo
echo -e "\x1B[38;2;17;245;120m################################################################################\x1b[0m"
echo -e "\x1B[38;2;17;245;120m$ time ./truncar disco 1 0\x1b[0m"
gdb -tui --args ./truncar disco 1 0
echo
echo -e "\x1B[38;2;17;245;120m$ ./leer_sf disco\x1b[0m"
echo -e "\x1B[38;2;17;245;120m#mostramos solo el SB\x1b[0m"
./leer_sf disco
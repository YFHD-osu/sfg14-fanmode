#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/io.h>
#include <string.h>

#define EC_CMD_PORT  0x66
#define EC_DATA_PORT 0x62
#define EC_OFFSET 0x45 // Write Reg 45 in EC RAM

void show_help() {
    printf(
        "Usage: nano [OPTIONS] [VALUE]\n"
        "\n"
        "Option    Meaning\n"
        " -h       Show this help message\n"
        " -r       Read current fan mode value from EC\n"
        " -w       Write fan mode to EC\n"
        "\n"
        "Writable value:\n"
        " 1        Balance Mode\n"
        " 2        Slient Mode\n"
        " 3        Permormance Mode\n"
    );
    
    return;
}

void wait_input_buffer_empty() {
    while (inb(EC_CMD_PORT) & 0x02); // Bit 1: Input buffer full
}

void wait_output_buffer_full() {
    while (!(inb(EC_CMD_PORT) & 0x01)); // Bit 0 = output buffer full
}

void ec_write(unsigned char value) {
    // 取得對 I/O 埠的存取權限
    if (ioperm(EC_CMD_PORT, 1, 1) || ioperm(EC_DATA_PORT, 1, 1)) {
        perror("ioperm");
        exit(1);
    }

    // 依序寫入：命令、位址、資料
    wait_input_buffer_empty();
    outb(0x81, EC_CMD_PORT);         // 0x81 = EC RAM Write

    wait_input_buffer_empty();
    outb(EC_OFFSET, EC_DATA_PORT);     // 寫入 offset

    wait_input_buffer_empty();
    outb(value, EC_DATA_PORT);      // 寫入資料
}

unsigned char ec_read() {
    // 取得 I/O 權限
    if (ioperm(EC_CMD_PORT, 1, 1) || ioperm(EC_DATA_PORT, 1, 1)) {
        perror("ioperm");
        return 0xFF;
    }

    wait_input_buffer_empty();
    outb(0x80, EC_CMD_PORT); // 0x80 = EC RAM Read

    wait_input_buffer_empty();
    outb(EC_OFFSET, EC_DATA_PORT); // 寫入 offset

    wait_output_buffer_full();
    return inb(EC_DATA_PORT); // 讀出資料
}

int main(int argc, char *argv[]) {

    if (argc < 2 || argc > 3) {
        printf(
            "Incorrect usage!\n"
            "Use %s -h to show help message:\n",
        argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0) {
        show_help();
        return 0;
    }

    if (strcmp(argv[1], "-w") == 0) {
        if (argc < 3) {
            printf("Incorrect usage! Write value not specified.\n");
            return 1;
        }

        unsigned char value = (unsigned char) strtol(argv[2], NULL, 16);

        if (value < 1 || value > 3) {
            printf("Incorrect usage! The write value must be 1, 2 or 3.\n");
            return 1;
        }

        ec_write(value);
        printf("Successfully set EC RAM 0x%02X to 0x%02X\n", EC_OFFSET, value);

        return 0;
    }

    if (strcmp(argv[1], "-r") == 0) {
        
        unsigned char value = ec_read();

        printf(
            "{\n"
            "  \"value\": \"%d\"\n"
            "}\n",
        value);
        
        return 0;
    }

    show_help();

    return 0;
}
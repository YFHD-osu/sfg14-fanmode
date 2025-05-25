#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/io.h>
#include <string.h>
#include <locale.h>

#define EC_CMD_PORT  0x66
#define EC_DATA_PORT 0x62
#define EC_OFFSET 0x45 // Write Reg 45 in EC RAM

#define BALANCE_MODE 1
#define SLIENT_MODE 2
#define PERFORMANCE_MODE 3

typedef enum opt_format {
    waybar,
    json
} opt_format_t;

typedef enum op_format {
    help,
    readEC,
    writeEC,
    toggleEC
} op_format_t;

typedef struct args {
    unsigned char writeValue;
    op_format_t opMode;
    opt_format_t format;
} args_t;

void showHelp(char *path) {
    printf(
        "Usage: %s [OPTIONS] [VALUE]\n"
        "\n"
        "Option    Meaning\n"
        " -h       Show this help message\n"
        " -r       Read current fan mode value from EC\n"
        " -w       Write fan mode to EC\n"
        "\n"
        "Writable value:\n"
        " %d        Balance Mode\n"
        " %d        Slient Mode\n"
        " %d        Permormance Mode\n",
    path, BALANCE_MODE, SLIENT_MODE, PERFORMANCE_MODE);
    
    return;
}

void wait_input_buffer_empty() {
    while (inb(EC_CMD_PORT) & 0x02); // Bit 1: Input buffer full
}

void wait_output_buffer_full() {
    while (!(inb(EC_CMD_PORT) & 0x01)); // Bit 0 = output buffer full
}

void ecWrite(unsigned char value) {
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

unsigned char ecRead() {
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

args_t* parseArgs(int argc, char *argv[]) {
    static args_t args = {help, 99, 99};

    char specfiedFormat = 0;
    char specfiedValue = 0;

    for (int i=1; i<argc; i++) {
        if (specfiedValue) {
            unsigned char value = (unsigned char) strtol(argv[i], NULL, 16);

            if (value < 1 || value > 3) {
                printf("Error: %d is not a valid value to write\n", value);
                return NULL;
            }

            specfiedValue = 0;
            args.writeValue = value;
            continue;
        }
        
        if (specfiedFormat) {
            if (strcmp("waybar", argv[i]) == 0) {
                args.format = waybar;
            } else if (strcmp("json", argv[i]) == 0) {
                args.format = json;
            } else {
                printf("%s is not a valid format.", argv[i]);
                return NULL;
            }

            specfiedFormat = 0;
            continue;
        }

        if (strcmp("-h", argv[i]) == 0) {
            args.opMode = help;
            return &args;
        }

        if (strcmp("-w", argv[i]) == 0) {
            if (args.opMode != 99) {
                printf("You cannot do multiple operation at the same time");
                return NULL;
            }
            
            specfiedValue = 1;
            args.opMode = writeEC;
            continue;
        }

        if (strcmp("-r", argv[i]) == 0) {
            if (args.opMode != 99) {
                printf("You cannot do multiple operation at the same time");
                return NULL;
            }

            args.opMode = readEC;
            continue;
        }

        if (strcmp("-f", argv[i]) == 0) {
            specfiedFormat = 1;
            continue;
        }

        if (strcmp("-t", argv[i]) == 0) {
            if (args.opMode != 99) {
                printf("You cannot do multiple operation at the same time");
                return NULL;
            }

            args.opMode = toggleEC;
            continue;
        }

    }

    if (specfiedValue) {
        printf("Write value is not specified\n");
        return NULL;
    }

    return &args;

}

void printWaybar(unsigned char value) {
    char *icons[] = {"󰾅", "󰾆", "󰓅"};
    char *tooltips[] = {
        "Balance", "Slient", "Performance"
    };

    printf("{\"value\": %d, \"text\": \"%s\", \"tooltip\": \"Fan Mode: %s\"}\n",
        value,
        icons[(value-1)%3],
        tooltips[(value-1)%3]
    );
}

int main(int argc, char *argv[]) {
    // char *argvss[] = {"utils", "-f", "waybar", "-r"};
    args_t *args = parseArgs(argc, argv);

    if (args == NULL) {
        printf("Use %s -h to show help message\n", argv[0]);
        return 1;
    }

    unsigned char value;
    unsigned char newValue;
    switch ((*args).opMode) {

        case writeEC:
            ecWrite((*args).writeValue);
            printf("Successfully set EC RAM 0x%02X to 0x%02X\n", EC_OFFSET, (*args).writeValue);
            return 0;

        case toggleEC:
            value = ecRead();

            if (value == 0xFF) {
                printf("Error reading the EC value\n");
                return 1;
            }

            switch (value) {
                case BALANCE_MODE: 
                    newValue = PERFORMANCE_MODE;
                    break;
                case SLIENT_MODE: 
                    newValue = BALANCE_MODE;
                    break;
                case PERFORMANCE_MODE:
                    newValue = SLIENT_MODE;
                    break;
            }
            ecWrite(newValue);

        case readEC:
            value = ecRead();

            if (value == 0xFF) {
                printf("Error reading the EC value\n");
                return 1;
            }

            if ((*args).format == waybar) {
                printWaybar(value);
                return 0;
            }

            printf("EC RAM value=%d\n", value);
            return 0;

        default:
            showHelp(argv[0]);
    }

    return 0;
}
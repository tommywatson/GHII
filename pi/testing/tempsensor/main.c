#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(*array))

const int device = 0x40;    // device address

const uint8_t cmd_reset[] = { 0xFE };
const uint8_t cmd_rtemp[] = { 0xE0 };
const uint8_t cmd_rhumidity[] = { 0xE5 };


/* Print an error */
void error(char *text) {
    fprintf(stderr,"%s\n",text);
}

/* write some data to the i2c device */
int i2c_write(int fd,const uint8_t *data,uint32_t length) {
    int count=-1;

    if(data) {
        count=write(fd,data,length);
        if(count!=length) {
            fprintf(stderr,
                    "Write failed [%d:%d] %d:%s\n",
                    count,
                    length,
                    errno,
                    strerror(errno));
        }
    }
    return count;
}

/* read some data from the i2c device */
int i2c_read(int fd,uint8_t *data,uint32_t length) {
    int count=-1;

    if(data) {
        count=read(fd,data,length);
#ifdef DEBUG
        if(count!=length) {
            fprintf(stderr,
                    "Read failed [%d:%d] %d:%s\n",
                    count,
                    length,
                    errno,
                    strerror(errno));
        }
#endif
    }
    return count;
}

/* try to open the i2c device */
int i2c_open() {
    int fd=-1;
    uint8_t data[2] = { 0x84, 0xB8 };
    char buffer[80];

    for(int i=0;i<2;++i) {
        snprintf(buffer,80,"/dev/i2c-%d",i);
        fd=open(buffer,O_RDWR);
        if(fd!=-1) {
            if(ioctl(fd,I2C_SLAVE,device)==0) {
                if(2==i2c_write(fd,data,sizeof(data))) {
                    if(1==i2c_read(fd,data,1)) {
                        fprintf(stderr,"Firmware 0x%0x\n",*data);
                    }
                }
                else {
                    error("Failed to query firmware");
                }
            }
            else {
                error("Failed to set slave");
                close(fd);
                fd=-1;
            }
        }
    }

    if(fd<0) {
        error("No I2C device file");
    }
    return fd;
}

/* get the humidity from the Si7021, returns 0 on error */
float get_humidity(int fd) {
    uint8_t data[2];
    float temp=0;

    if(i2c_write(fd,
                 cmd_rhumidity,
                 ARRAY_SIZE(cmd_rhumidity))==ARRAY_SIZE(cmd_rhumidity)) {
        // while the unit is still sampling and less then 10 tries
        for(uint8_t tries=0;tries<10&&i2c_read(fd,data,1)!=1;++tries,sleep(1));
        // now read the next byte
        if(i2c_read(fd,data+1,1)==1) {
            uint16_t code=(*data<<8)+*(data+1);
            temp=125*code/65536-6;
        }
        else {
            error("Failed to read humidity");
        }
    }
    else {
        error("Failed to request humidity");
    }
    return temp;
}

/* get the temp from the Si7021, returns 0 on error */
float get_temp(int fd) {
    uint8_t data[2];
    float temp=0;

    if(i2c_write(fd,cmd_rtemp,ARRAY_SIZE(cmd_rtemp))==ARRAY_SIZE(cmd_rtemp)) {
        if(i2c_read(fd,data,ARRAY_SIZE(data))==ARRAY_SIZE(data)) {
            uint16_t code=(*data<<8)+*(data+1);
            temp=175.72*code/65536-46.85;
        }
        else {
            error("Failed to read temp");
        }
    }
    else {
        error("Failed to request temp");
    }
    return temp;
}

int main(void) {
    float temp,humidity;
    int fd=i2c_open();
    char cmd[256];

    if(fd!=-1) {
        while(1) {
            humidity=get_humidity(fd);
            if(humidity>0) {
                temp=get_temp(fd);
                sprintf(cmd,
                        "curl -s --data \"date=%ld&temp=%f&humidity=%f\" "
                        "http://tommywatson.com/GHII/temp.php",
                        time(0),
                        temp,
                        humidity);
                system(cmd);
                fprintf(stderr,"%s",cmd);
                sleep(60);
            }
        }
        close(fd);
    }
    else {
        error("Cannot open I2C");
    }
    return 0;
}


#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "rp.h"
#include "common.h"


void waitForUser ( void )
{
    int ch;
    struct termios oldt, newt;

    puts("Press 'c' to continue or 'q' to exit.");
    
    tcgetattr ( STDIN_FILENO, &oldt );
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr ( STDIN_FILENO, TCSANOW, &newt );

    do {
        ch = getc(stdin);
    } while((ch != 'C') && (ch != 'c') && (ch != 'Q') && (ch != 'q'));
    
    tcsetattr ( STDIN_FILENO, TCSANOW, &oldt );
    
    if((ch == 'Q') || (ch == 'q'))
        exit(1);
}

void backupParams() {
    rp_calib_params_t calib = rp_GetCalibrationSettings();
    FILE* fout = fopen("calib_params.dat", "wb");
    fwrite(&calib, sizeof(rp_calib_params_t), 1, fout);
    fclose(fout);
}

void printParams(const rp_calib_params_t* calib) {
    rp_calib_params_t _calib;
    if(!calib) {
        _calib = rp_GetCalibrationSettings();
        calib = &_calib;
    }
    
    printf("Current calibration params:\n");
    printf("fe_ch1_fs_g_hi = 0x%08X //!< High gain front end full scale voltage, channel A\n", calib->fe_ch1_fs_g_hi);
    printf("fe_ch2_fs_g_hi = 0x%08X //!< High gain front end full scale voltage, channel B\n", calib->fe_ch2_fs_g_hi);
    printf("fe_ch1_fs_g_lo = 0x%08X //!< Low gain front end full scale voltage, channel A\n", calib->fe_ch1_fs_g_lo);
    printf("fe_ch2_fs_g_lo = 0x%08X //!< Low gain front end full scale voltage, channel B\n", calib->fe_ch2_fs_g_lo);
    
    printf("fe_ch1_dc_offs = 0x%08X //!< Front end DC offset, channel A\n", calib->fe_ch1_dc_offs);
    printf("fe_ch2_dc_offs = 0x%08X //!< Front end DC offset, channel B\n", calib->fe_ch2_dc_offs);
    
    printf("be_ch1_fs = 0x%08X //!< Back end full scale voltage, channel A\n", calib->be_ch1_fs);
    printf("be_ch2_fs = 0x%08X //!< Back end full scale voltage, channel B\n", calib->be_ch2_fs);
    printf("be_ch1_dc_offs = 0x%08X //!< Back end DC offset, on channel A\n", calib->be_ch1_dc_offs);
    printf("be_ch2_dc_offs = 0x%08X //!< Back end DC offset, on channel B\n", calib->be_ch2_dc_offs);
}

int restoreParams(const char* path) {
    rp_calib_params_t calib;
    FILE* fin = fopen(path, "rb");
    if(!fin)
        return -1;
    
    if(fread(&calib, sizeof(rp_calib_params_t), 1, fin) != 1) {
        fclose(fin);
        return -1;
    }
    
    fclose(fin);
    
    ECHECK(rp_CalibrationWriteParams(calib));
    printParams(&calib);
    return 0;
}

int main(int argc, char **argv) {
    float value;
    int ret;
    printf("Library version: %s\n", rp_GetVersion());

    ECHECK(rp_Init());
    
    if(argc == 2) {
        ECHECK(restoreParams(argv[1]));
        ECHECK(rp_Release());
        return 0;
    }
    
    ECHECK(rp_CalibrationReset());

    puts("---Calibration application---\n");

    puts("Calibration proces started.");

    puts("Connect CH1 to ground.");
    waitForUser();
    ECHECK(rp_CalibrateFrontEndOffset(RP_CH_1, NULL));

    do {
        puts("Connect CH1 to reference voltage source and set jumpers to HV.");
        puts("Enter reference voltage: ");
        ret = scanf("%f", &value);
    } while ((ret != 1) && (value <= 0.f) && (value > 20.f));
    printf("Calibrating to %f V\n", value);
    ECHECK(rp_CalibrateFrontEndScaleHV(RP_CH_1, value, NULL));

    do {
        puts("Connect CH1 to reference voltage source and set jumpers to LV.");
        puts("Enter reference voltage: ");
        ret = scanf("%f", &value);
    } while ((ret != 1) && (value <= 0.f) && (value > 1.f));
    printf("Calibrating to %f V\n", value);
    ECHECK(rp_CalibrateFrontEndScaleLV(RP_CH_1, value, NULL));

    puts("Connect CH1 Outout to CH1 Input. Press any key to continue.");
    waitForUser();
    ECHECK(rp_CalibrateBackEnd(RP_CH_1, NULL));

    puts("Connect CH2 to ground.");
    waitForUser();
    ECHECK(rp_CalibrateFrontEndOffset(RP_CH_2, NULL));

    do {
        puts("Connect CH2 to reference voltage source and set jumpers to HV.");
        puts("Enter reference voltage: ");
        ret = scanf("%f", &value);
    } while ((ret != 1) && (value <= 0.f) && (value > 20.f));
    printf("Calibrating to %f V\n", value);
    ECHECK(rp_CalibrateFrontEndScaleHV(RP_CH_2, value, NULL));

    do {
        puts("Connect CH2 to reference voltage source and set jumpers to LV.");
        puts("Enter reference voltage: ");
        ret = scanf("%f", &value);
    } while ((ret != 1) && (value <= 0.f) && (value > 1.f));
    printf("Calibrating to %f V\n", value);
    ECHECK(rp_CalibrateFrontEndScaleLV(RP_CH_2, value, NULL));

    puts("Connect CH2 Outout to CH2 Input.");
    waitForUser();
    ECHECK(rp_CalibrateBackEnd(RP_CH_2, NULL));

    printParams(NULL);
    backupParams();
    
    ECHECK(rp_Release());
    return 0;
}


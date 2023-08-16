#ifndef RVMN_SPI_H
#define RVMN_SPI_H

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include <stdint.h>
#include <errno.h>
//#include <misc/printk.h>
#include <zephyr/device.h>
//#include <gpio.h>
#include <errno.h>
#include <zephyr/kernel.h>
//#include <init.h>
//#include <soc.h>
//#include <sys_io.h>
//#include <misc/util.h>
//#include <spi.h>

#define BUFF_LENGTH 			600        	/**< Transfer length. */
#define NUMBER_OF_XFERS 		3
#define BYTES_PR_XFER   		600/*200*/ 		/* Number of bytes per transfer */
#define SPI_TRANSFER_SIZE 600

#define SPIM_DRV_NAME 			"SPI_0"
#define SPI_CS_PIN      		31
#define SPI_READY_PIN_IN        19
#define SPI_FREQUENCY 			/*8000000U*/1000000

/* Inter-Processor Interface Packet Structure Info */
#define IPI_PSINFO(MAJOR, MINOR) 	((MAJOR << 8) | (MINOR))

/* Invalid entry used at 1st SPI transfer */
#define IPI_PSINFO_V0_0			IPI_PSINFO(0, 0)

#define IPI_PSINFO_V1_4			IPI_PSINFO(1, 4)
#define IPI_PSINFO_V3_0			IPI_PSINFO(3, 0)
#define IPI_PSINFO_V4_0			IPI_PSINFO(4, 0)

/* TMC CR0003 - IGN_MON + parking brake */
#define IPI_PSINFO_V5_0			IPI_PSINFO(5, 0)

/* NXP v1.9 reports this packet structure */
#define IPI_PSINFO_V1_2			IPI_PSINFO(1, 2)

/* The following settings are specific to BMPRO Connect*/
#define INPUT_CNT 8
#define IPI_PS_V1_2_SUPPORTED true
#define IPI_PS_V1_4_SUPPORTED true
#define IPI_PS_V3_0_SUPPORTED false
#define IPI_PS_V4_0_SUPPORTED true
#define IPI_PS_V5_0_SUPPORTED false

typedef union {
    struct {
        uint32_t    sa             :  8;
        uint32_t    dgn            : 18;
        uint32_t    pri            :  3;
        uint32_t    validity       :  3;       /* this is not a part of the RV-C standard but that of Setec CAN-SPI protocol */
    } __attribute__ ((packed));             /* RV-C id as per J1939 + 3 validity bits at bit 29-31   */
    uint16_t    canid[2];                      /* standard 29-bit CAN id + 3 validity bits at bit 29-31 */
} RVC_tunId;

// CAN Message Structure
typedef struct CAN_stMsg
{
    union {
        struct {
            uint16_t u16ID15_0;  // ID bits 0 to 15
            uint16_t u16ID28_16; // Message valid bit, Extend identifier and direction bits, ID bits 16 to 28
        };
        RVC_tunId id;
    };

    union {
        uint16_t u16Data[4];     // Data (two bytes as in the CAN message interface data registers)
        uint8_t u8Data[8];       // Data (single bytes as shown in the RVMCwithLEDDsiplay_V1_1.docx document)
    };
    uint8_t  u8DLC;      // DLC (bit0 to 3 only)
    uint8_t  u8CS;       // Message Checksum
} __attribute__ ((packed)) CAN_tstMsg;

#define SPIS_nMDatasize 600             // Master Data size in bytes
#define SPIS_nSDatasize SPIS_nMDatasize // Slave  Data size in bytes must be same as Master

#define SPIS_nOutp_0 	8	// Number of Outputs Full Bridge
#define SPIS_nOutp_1 	4	// Number of Outputs HSD 10A
#define SPIS_nOutp_2 	8	// Number of Outputs HSD Dimmable 5A
#define SPIS_nOutp_3 	18	// Number of Outputs HSD 500mA
#define SPIS_nOutp_4 	4	// Number of Outputs LSD 500mA
#define SPIS_nOutp_5 	6	// Number of Outputs HSD 5A (not dimmable)

#define SPIS_nInp_0	8	// Byte count only, digital input packing depends on IPI PS structure version

#define SPIS_nInp_1     14  // Number of Inputs A2D

#define SPIS_nCANMsgRX  32  // Number of CAN Messages received
#define SPIS_niCANMsgSWITCHSTATUS 0 // The SWITCH_STATUS message from RVMC is the first message in the array/

struct output_monitor {
	uint32_t state : 2;
	uint32_t err_type : 2;
	uint32_t current_ma : 28; /* mA */
};

int RVM_spi_get_pwm_info(uint8_t on_off_i, struct output_monitor *om);

//
// Type Definition of Data from Slave
//
typedef struct SPIS_tuMISO { // Slave Data Structure as defined in RVMN_InterProcessorInterface_v1.5.xlsx
	union {
		struct {
			// Info
			uint16_t u16PSinfo; // Packet Structure Info (Hbyte = Major, LByte = Minor Version)
			uint16_t u16SwVer; // Software Version
			uint32_t u32RollCnt; // Rolling Counter, roll over after reaching max value.
			uint32_t u32CSinfo; // Block Checksum (Info)
			struct { // I/O Sub-Structure
				uint16_t u16Outp_0[SPIS_nOutp_0]; // Outputs Full Bridge
				uint16_t u16Outp_1[SPIS_nOutp_1]; // Outputs HSD 10A
				union {
					uint32_t u32Outp_2[SPIS_nOutp_2]; // Outputs HSD Dimmable 5A
					uint16_t u16Outp_2[SPIS_nOutp_2 * 2]; // Outputs HSD Dimmable 5A
				};
				uint8_t u8Outp_3[SPIS_nOutp_3]; // Outputs HSD 500mA
				uint8_t u8Outp_4[SPIS_nOutp_4]; // Outputs LSD 500mA
				uint8_t u8Outp_5[SPIS_nOutp_5]; // Outputs HSD 5A (not dimmable)

				uint8_t u8Inp_0[SPIS_nInp_0]; // Inputs Digital
				uint16_t u16Inp_1[SPIS_nInp_1] __attribute__((packed)); // Inputs A2D
				uint32_t u32CS __attribute__((packed)); // Block Checksum (I/O Sub-Structure)
			} __attribute__((packed)) IO; // IO Status

			CAN_tstMsg aCANMsg[SPIS_nCANMsgRX]; // CAN Messages received, SWITCH_STATUS from RVMC is first element.
			// System Info
			// Median (5) filtered A2D value, A2D-Value: bit[0..11] Reserved: bit[12..15] (set to 0)
			uint16_t u16VBATT_MON; // Battery Voltage
			uint16_t u16VCC_Switched; // Sensor Supply Voltage
			uint16_t u16DETECT_VHBRIDGE; // H-Brideg Supply Voltage
			uint16_t u16SysInfoReserved; // Reserved (set to 0)
			uint16_t ign_mon_adc; // Ignition Monitor (analogue) currently only supported by PS v5.0

			uint8_t u8Reserve[2]; // Reserved, set to 0.

			uint32_t u32CS __attribute__((packed)); // Block Checksum (Overall)
		};
		struct {
			uint8_t spi_dma_block[SPI_TRANSFER_SIZE];
			/*
			uint8_t spi_dma_block0[BYTES_PR_XFER];
			uint8_t spi_dma_block1[BYTES_PR_XFER];
			uint8_t spi_dma_block2[BYTES_PR_XFER];
			*/

		} __attribute__((packed));
	};
} __attribute__((packed)) SPIS_tuMISO;

#define SPIS_nOCtp_0 	2	// Number of bytes for Output Control Full Bridge
#define SPIS_nOCtp_1 	1	// Number of bytes for Outputs HSD 10A
#define SPIS_nOCtp_2 	16	// Number of bytes for Outputs HSD Dimmable 5A
#define SPIS_nOCtp_3 	3	// Number of bytes for Outputs HSD 500mA
#define SPIS_nOCtp_4 	1	// Number of bytes for Outputs LSD 500mA
#define SPIS_nOCtp_5 	1	// Number of bytes for Outputs HSD 5A (newer hardware is dimmable)

#define SPIS_nOCtp_mask_2 	1	// Number of bytes for Mask Outputs HSD Dimmable 5A
#define SPIS_nOC_reserved 	2	// Number of bytes Reserved

#define SPIS_nCANMsgTX  4       // Number of CAN Messages

//
// Type Definition of Data from Master
//
typedef struct SPIS_tuMOSI { // Master Data Structure as defined in RVMN_InterProcessorInterface_v1.5.xlsx
	union {
		struct {
			// Info
			uint16_t u16PSinfo; // Packet Structure Info
			uint8_t version_minor;
			uint8_t version_major;
			uint32_t u32RollCnt; // Rolling Counter, roll over after reaching max value.
			uint32_t u32CSinfo; // Block Checksum (Info)
			struct { // Output Control Sub-Structure
				uint8_t u8OCtp_0[SPIS_nOCtp_0]; // Output for Full Bridges (2bits/output)
				uint8_t u8OCtp_1[SPIS_nOCtp_1]; // Outputs HSD 10A (1bit/output)
				uint16_t u16OCtp_2[(SPIS_nOCtp_2 / 2)] __attribute__((packed)); // Outputs HSD Dimmable 5A, PWM setting 0 to 65535
				uint8_t u8OCtp_3[SPIS_nOCtp_3]; // Outputs HSD 500mA (1bit/output)
				uint8_t u8OCtp_4[SPIS_nOCtp_4]; // Outputs LSD 500mA (1bit/output)
				uint8_t u8OCtp_5[SPIS_nOCtp_5]; // Outputs HSD 5A (1bit/output)
				uint8_t u8OCtp_mask_0[SPIS_nOCtp_0]; // Mask for Full Bridges (2bits/output)
				uint8_t u8OCtp_mask_1[SPIS_nOCtp_1]; // Mask outputs HSD 10A (1bit/output)
				uint8_t u8OCtp_mask_2[SPIS_nOCtp_mask_2]; // MAsk outputs HSD Dimmable 5A, PWM setting 0 to 65535
				uint8_t u8OCtp_mask_3[SPIS_nOCtp_3]; // MAsk outputs HSD 500mA (1bit/output)
				uint8_t u8OCtp_mask_4[SPIS_nOCtp_4]; // MAsk outputs LSD 500mA (1bit/output)
				uint8_t u8OCtp_mask_5[SPIS_nOCtp_5]; // MAsk outputs HSD 5A (1bit/output)

				uint8_t u8OCHBRlyOffDly; // H-Bridge relay turn-off delay
				uint8_t u8OC_reserved[SPIS_nOC_reserved]; // Reserved for future use

				uint32_t u32CS __attribute__((packed)); // Sub-Structure Checksum
			} __attribute__((packed)) OC; // End of Output Control Sub-Structure

			struct { // Output Configuration Sub-Structure
				// Full Bridges
				uint8_t u8OCfgstalltp_0[SPIS_nOutp_0]; // Stall current threshold, 250mA resolution
				uint8_t u8OCfgstalldlytp_0[SPIS_nOutp_0]; // Stall current switch off delay, 50ms resolution
				// HSD 10A
				uint8_t u8OCfgovertp_1[SPIS_nOutp_1]; // Over current threshold, 250mA resolution
				// HSD Dimmable 5A
				uint8_t u8OCfgovertp_2[SPIS_nOutp_2]; // Over current threshold, 250mA resolution
				// HSD 500mA
				uint8_t u8OCfgovertp_3[SPIS_nOutp_3]; // Over current threshold, 250mA resolution
				// LSD 500mA
				uint8_t u8OCfgovertp_4[SPIS_nOutp_4]; // Over current threshold, 250mA resolution
				// HSD 5A (not dimmable)
				uint8_t u8OCfgovertp_5[SPIS_nOutp_5]; // Over current threshold, 250mA resolution

				uint32_t u32CS __attribute__((packed)); // Sub-Structure Checksum
			} __attribute__((packed)) OCfg; // End of Output Configuration Sub-Structure

			CAN_tstMsg aCANMsg[SPIS_nCANMsgTX]; // CAN Messages to be sent

			// System Info
			uint8_t u8LowPowerModeInfo; // bit0: LPM1 (ECO mode) status - 0b = OFF; 1b = ACTIVE
				// bit1: LPM2 (Storage mode) status - 0b = OFF; 1b = ACTIVE
				// bit[2..7]: Reserved (set to 0)
			uint8_t u8USBPowerCmd;

			struct {
				uint16_t u16OCtp_5_pwm[SPIS_nOutp_5];
				uint32_t u32CS; // Sub-Structure Checksum
			} __attribute__((packed)) pwm1_ctl;
			uint8_t u8Reserve[410]; // Reserved, set to 0.
			uint32_t u32CS __attribute__((packed)); // Block Checksum
		};
		struct {
			/*
			uint8_t spi_dma_block0[BYTES_PR_XFER];
			uint8_t spi_dma_block1[BYTES_PR_XFER];
			uint8_t spi_dma_block2[BYTES_PR_XFER];
			*/
			uint8_t spi_dma_block[SPI_TRANSFER_SIZE];

		} __attribute__((packed));
	};
} __attribute__((packed)) SPIS_tuMOSI;

/* Transmit Buffer */
extern SPIS_tuMOSI SPIS_stuMData; // Data from Master

/* Receive Buffer */
extern SPIS_tuMISO SPIS_stuSData; // Data from Slave


/* Global spi semaphore */
extern struct k_sem g_spi_sem;


int 		iSpi_init(void);
uint32_t 	u32CalculateCS(const uint32_t *data, uint16_t len);
int RVM_spi_transceive(void);
uint16_t rvmn_spi_get_ps_info(void);

#endif // #ifndef RVMN_SPI_H

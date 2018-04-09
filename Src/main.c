/*
 / _____)             _              | |
 ( (____  _____ ____ _| |_ _____  ____| |__
 \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 _____) ) ____| | | || |_| ____( (___| | | |
 (______/|_____)_|_|_| \__)_____)\____)_| |_|
 (C)2013 Semtech

 Description: LoRaMac classC device implementation

 License: Revised BSD License, see LICENSE.TXT file include in the project

 Maintainer: Andreas Pella (IMST GmbH), Miguel Luis and Gregory Cristian
 */
#include <string.h>
#include <math.h>
#include <time.h>
#include "board.h"

#include "LoRaMac.h"
#include "LoRaMacCrypto.h"
#include "uart-board.h"
#include "Commissioning.h"

#include <pb_encode.h>
#include <pb_decode.h>
#include "weather.pb.h"


/*!
 * Defines the application data transmission duty cycle. 5s, value in [ms].
 */
#define APP_TX_DUTYCYCLE                            30000

/*!
 * Defines a random delay for application data transmission duty cycle. 1s,
 * value in [ms].
 */
#define APP_TX_DUTYCYCLE_RND                        1000

/*!
 * Default datarate
 */
#define LORAWAN_DEFAULT_DATARATE                    DR_0

/*!
 * LoRaWAN confirmed messages
 */
#define LORAWAN_CONFIRMED_MSG_ON                    false

/*!
 * LoRaWAN Adaptive Data Rate
 *
 * \remark Please note that when ADR is enabled the end-device should be static
 */
#define LORAWAN_ADR_ON                              1

#if defined( USE_BAND_868 )

#include "LoRaMacTest.h"

/*!
 * LoRaWAN ETSI duty cycle control enable/disable
 *
 * \remark Please note that ETSI mandates duty cycled transmissions. Use only for test purposes
 */
#define LORAWAN_DUTYCYCLE_ON                        false

#define USE_SEMTECH_DEFAULT_CHANNEL_LINEUP          0

#if( USE_SEMTECH_DEFAULT_CHANNEL_LINEUP == 1 )

#define LC4                { 867100000, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC5                { 867300000, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC6                { 867500000, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC7                { 867700000, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC8                { 867900000, { ( ( DR_5 << 4 ) | DR_0 ) }, 0 }
#define LC9                { 868800000, { ( ( DR_7 << 4 ) | DR_7 ) }, 2 }
#define LC10               { 868300000, { ( ( DR_6 << 4 ) | DR_6 ) }, 1 }

#endif

#endif

/*!
 * LoRaWAN application port
 */
#define LORAWAN_APP_PORT                            3

/*!
 * User application data buffer size
 */
#define LORAWAN_APP_DATA_SIZE                       4

static uint8_t DevEui[] = LORAWAN_DEVICE_EUI;
static uint8_t AppEui[] = LORAWAN_APPLICATION_EUI;
static uint8_t AppKey[] = LORAWAN_APPLICATION_KEY;

#if( OVER_THE_AIR_ACTIVATION == 0 )

		static uint8_t NwkSKey[] = LORAWAN_NWKSKEY;
		static uint8_t AppSKey[] = LORAWAN_APPSKEY;

		/*!
		 * Device address
		 */
		static uint32_t DevAddr = LORAWAN_DEVICE_ADDRESS;

#endif

		/*!
		 * Application port
		 */
		static uint8_t AppPort = LORAWAN_APP_PORT;

		/*!
		 * User application data size
		 */
		uint8_t AppDataSize ;

		/*!
		 * User application data buffer size
		 */
#define LORAWAN_APP_DATA_MAX_SIZE                           64

/*!
 * User application data
 */
static uint8_t AppData[LORAWAN_APP_DATA_MAX_SIZE];

/*!
 * Indicates if the node is sending confirmed or unconfirmed messages
 */
static uint8_t IsTxConfirmed = LORAWAN_CONFIRMED_MSG_ON;

/*!
 * Defines the application data transmission duty cycle
 */
static uint32_t TxDutyCycleTime;

/*!
 * Timer to handle the application data transmission duty cycle
 */
static TimerEvent_t TxNextPacketTimer;

/*!
 * Specifies the state of the application LED
 */
static bool AppLedStateOn = false;
/*!
 * Indicates if a new packet can be sent
 */
static bool NextTx = true;

/*!
 * Device states
 */
static enum eDeviceState
{
	DEVICE_STATE_INIT,
	DEVICE_STATE_JOIN,
	DEVICE_STATE_SEND,
	DEVICE_STATE_CYCLE,
	DEVICE_STATE_SLEEP,
}DeviceState;



#define MFR_LEN   4
 /*!
 * LoRaWAN compliance tests support data
 */
struct ComplianceTest_s
{
	bool Running;
	uint8_t State;
	bool IsTxConfirmed;
	uint8_t AppPort;
	uint8_t AppDataSize;
	uint8_t *AppDataBuffer;
	uint16_t DownLinkCounter;
	bool LinkCheck;
	uint8_t DemodMargin;
	uint8_t NbGateways;
}ComplianceTest;

//uint8_t encryptKey[16] = LORAWAN_APPLICATION_KEY;
//uint8_t dongleID[8] = LORAWAN_DEVICE_EUI;
uint8_t txBuffer[100];
uint8_t rxBuffer[100];
uint8_t sensVal[100];// prepare TX frame
uint8_t flag = 0;
uint8_t txFlag = 0;
uint16_t msgLen = 0;
uint16_t valueLen = 0; //prepare TX frame

volatile uint32_t Rotations;
volatile uint32_t ContactBounceTime;
float MPH;
float KmPH;
uint16_t vaneValue;
uint16_t direction;
uint16_t calDirection;
char windDirection[3];
char heading[3];

Gpio_t windSpeed;
Adc_t windVane;

#define SPEED										PB_15
#define VANE										PB_14
#define offset										0

void UartISR(UartNotifyId_t id);
void irqRotation();
uint16_t directionMap(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);
char getHeading(uint16_t direction);
uint8_t buffer[20];
pb_ostream_t stream;

/*!f
 * \brief   Prepares the payload of the frame
 */
static void PrepareTxFrame(uint8_t port) {
	switch (port) {
	case 3:{
//		weatherProto wt = {MPH, KmPH, calDirection, windDirection};
		weatherProto wt = {MPH, KmPH, calDirection};
		pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
		pb_encode(&stream, weatherProto_fields, &wt);
		memcpy(AppData, buffer, (uint8_t)stream.bytes_written);
		AppDataSize = (uint8_t)stream.bytes_written;
	}
		break;
	case 224:
		if (ComplianceTest.LinkCheck == true) {
			ComplianceTest.LinkCheck = false;
			AppDataSize = 3;
			AppData[0] = 5;
			AppData[1] = ComplianceTest.DemodMargin;
			AppData[2] = ComplianceTest.NbGateways;
			ComplianceTest.State = 1;
		} else {
			switch (ComplianceTest.State) {
			case 4:
				ComplianceTest.State = 1;
				break;
			case 1:
				AppDataSize = 2;
				AppData[0] = ComplianceTest.DownLinkCounter >> 8;
				AppData[1] = ComplianceTest.DownLinkCounter;
				break;
			}
		}
		break;
	default:
		break;
	}
}

/*!
 * \brief   Prepares the payload of the frame
 *
 * \retval  [0: frame could be send, 1: error]
 */
static bool SendFrame(void) {
	McpsReq_t mcpsReq;
	LoRaMacTxInfo_t txInfo;

	if (LoRaMacQueryTxPossible(AppDataSize, &txInfo) != LORAMAC_STATUS_OK) {
		// Send empty frame in order to flush MAC commands
		mcpsReq.Type = MCPS_UNCONFIRMED;
		mcpsReq.Req.Unconfirmed.fBuffer = NULL;
		mcpsReq.Req.Unconfirmed.fBufferSize = 0;
		mcpsReq.Req.Unconfirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
	} else {
		if (IsTxConfirmed == false) {
			mcpsReq.Type = MCPS_UNCONFIRMED;
			mcpsReq.Req.Unconfirmed.fPort = AppPort;
			mcpsReq.Req.Unconfirmed.fBuffer = AppData;
			mcpsReq.Req.Unconfirmed.fBufferSize = AppDataSize;
			mcpsReq.Req.Unconfirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
		} else {
			mcpsReq.Type = MCPS_CONFIRMED;
			mcpsReq.Req.Confirmed.fPort = AppPort;
			mcpsReq.Req.Confirmed.fBuffer = AppData;
			mcpsReq.Req.Confirmed.fBufferSize = AppDataSize;
			mcpsReq.Req.Confirmed.NbTrials = 8;
			mcpsReq.Req.Confirmed.Datarate = LORAWAN_DEFAULT_DATARATE;
		}
	}

	if (LoRaMacMcpsRequest(&mcpsReq) == LORAMAC_STATUS_OK) {
		return false;
	}
	return true;
}

/*!
 * \brief Function executed on TxNextPacket Timeout event
 */
static void OnTxNextPacketTimerEvent(void) {
	MibRequestConfirm_t mibReq;
	LoRaMacStatus_t status;

	TimerStop(&TxNextPacketTimer);

	mibReq.Type = MIB_NETWORK_JOINED;
	status = LoRaMacMibGetRequestConfirm(&mibReq);

	if (status == LORAMAC_STATUS_OK) {
		if (mibReq.Param.IsNetworkJoined == true) {
			DeviceState = DEVICE_STATE_SEND;
			NextTx = true;
		} else {
			DeviceState = DEVICE_STATE_JOIN;
		}
	}
}

/*!
 * \brief   MCPS-Confirm event function
 *
 * \param   [IN] mcpsConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void McpsConfirm(McpsConfirm_t *mcpsConfirm) {
	if (mcpsConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK) {
		switch (mcpsConfirm->McpsRequest) {
		case MCPS_UNCONFIRMED: {
			// Check Datarate
			// Check TxPower
			break;
		}
		case MCPS_CONFIRMED: {
			// Check Datarate
			// Check TxPower
			// Check AckReceived
			// Check NbTrials
			break;
		}
		case MCPS_PROPRIETARY: {
			break;
		}
		default:
			break;
		}
	}
	NextTx = true;
}

/*!
 * \brief   MCPS-Indication event function
 *
 * \param   [IN] mcpsIndication - Pointer to the indication structure,
 *               containing indication attributes.
 */
static void McpsIndication(McpsIndication_t *mcpsIndication) {
	if (mcpsIndication->Status != LORAMAC_EVENT_INFO_STATUS_OK) {
		return;
	}

	switch (mcpsIndication->McpsIndication) {
	case MCPS_UNCONFIRMED: {
		break;
	}
	case MCPS_CONFIRMED: {
		break;
	}
	case MCPS_PROPRIETARY: {
		break;
	}
	case MCPS_MULTICAST: {
		break;
	}
	default:
		break;
	}

	// Check Multicast
	// Check Port
	// Check Datarate
	// Check FramePending
	// Check Buffer
	// Check BufferSize
	// Check Rssi
	// Check Snr
	// Check RxSlot

	if (ComplianceTest.Running == true) {
		ComplianceTest.DownLinkCounter++;
	}

	if (mcpsIndication->RxData == true) {
		switch (mcpsIndication->Port) {
		case 1:

			memcpy(rxBuffer, mcpsIndication->Buffer,
					mcpsIndication->BufferSize);


			break;

		case 2:
			if (mcpsIndication->BufferSize == 1) {
				AppLedStateOn = mcpsIndication->Buffer[0] & 0x01;
				GpioWrite(&Led3, ((AppLedStateOn & 0x01) != 0) ? 1 : 0);
			}
			break;
		case 224:
			if (ComplianceTest.Running == false) {
				// Check compliance test enable command (i)
				if ((mcpsIndication->BufferSize == 4)
						&& (mcpsIndication->Buffer[0] == 0x01)
						&& (mcpsIndication->Buffer[1] == 0x01)
						&& (mcpsIndication->Buffer[2] == 0x01)
						&& (mcpsIndication->Buffer[3] == 0x01)) {
					IsTxConfirmed = false;
					AppPort = 224;
					AppDataSize = 2;
					ComplianceTest.DownLinkCounter = 0;
					ComplianceTest.LinkCheck = false;
					ComplianceTest.DemodMargin = 0;
					ComplianceTest.NbGateways = 0;
					ComplianceTest.Running = true;
					ComplianceTest.State = 1;

					MibRequestConfirm_t mibReq;
					mibReq.Type = MIB_ADR;
					mibReq.Param.AdrEnable = true;
					LoRaMacMibSetRequestConfirm(&mibReq);

#if defined( USE_BAND_868 )
					LoRaMacTestSetDutyCycleOn( false);
#endif
				}
			} else {
				ComplianceTest.State = mcpsIndication->Buffer[0];
				switch (ComplianceTest.State) {
				case 0: // Check compliance test disable command (ii)
					IsTxConfirmed = LORAWAN_CONFIRMED_MSG_ON;
					AppPort = LORAWAN_APP_PORT;
					AppDataSize = LORAWAN_APP_DATA_SIZE;
					ComplianceTest.DownLinkCounter = 0;
					ComplianceTest.Running = false;

					MibRequestConfirm_t mibReq;
					mibReq.Type = MIB_ADR;
					mibReq.Param.AdrEnable = LORAWAN_ADR_ON;
					LoRaMacMibSetRequestConfirm(&mibReq);
#if defined( USE_BAND_868 )
					LoRaMacTestSetDutyCycleOn( LORAWAN_DUTYCYCLE_ON);
#endif
					break;
				case 1: // (iii, iv)
					AppDataSize = 2;
					break;
				case 2: // Enable confirmed messages (v)
					IsTxConfirmed = true;
					ComplianceTest.State = 1;
					break;
				case 3:  // Disable confirmed messages (vi)
					IsTxConfirmed = false;
					ComplianceTest.State = 1;
					break;
				case 4: // (vii)
					AppDataSize = mcpsIndication->BufferSize;

					AppData[0] = 4;
					for (uint8_t i = 1; i < AppDataSize; i++) {
						AppData[i] = mcpsIndication->Buffer[i] + 1;
					}
					break;
				case 5: // (viii)
				{
					MlmeReq_t mlmeReq;
					mlmeReq.Type = MLME_LINK_CHECK;
					LoRaMacMlmeRequest(&mlmeReq);
				}
					break;
				case 6: // (ix)
				{
					MlmeReq_t mlmeReq;

					// Disable TestMode and revert back to normal operation
					IsTxConfirmed = LORAWAN_CONFIRMED_MSG_ON;
					AppPort = LORAWAN_APP_PORT;
					AppDataSize = LORAWAN_APP_DATA_SIZE;
					ComplianceTest.DownLinkCounter = 0;
					ComplianceTest.Running = false;

					MibRequestConfirm_t mibReq;
					mibReq.Type = MIB_ADR;
					mibReq.Param.AdrEnable = LORAWAN_ADR_ON;
					LoRaMacMibSetRequestConfirm(&mibReq);
#if defined( USE_BAND_868 )
					LoRaMacTestSetDutyCycleOn( LORAWAN_DUTYCYCLE_ON);
#endif

					mlmeReq.Type = MLME_JOIN;

					mlmeReq.Req.Join.DevEui = DevEui;
					mlmeReq.Req.Join.AppEui = AppEui;
					mlmeReq.Req.Join.AppKey = AppKey;
					mlmeReq.Req.Join.NbTrials = 3;

					LoRaMacMlmeRequest(&mlmeReq);
					DeviceState = DEVICE_STATE_SLEEP;
				}
					break;
				case 7: // (x)
				{
					if (mcpsIndication->BufferSize == 3) {
						MlmeReq_t mlmeReq;
						mlmeReq.Type = MLME_TXCW;
						mlmeReq.Req.TxCw.Timeout =
								(uint16_t) ((mcpsIndication->Buffer[1] << 8)
										| mcpsIndication->Buffer[2]);
						LoRaMacMlmeRequest(&mlmeReq);
					} else if (mcpsIndication->BufferSize == 7) {
						MlmeReq_t mlmeReq;
						mlmeReq.Type = MLME_TXCW_1;
						mlmeReq.Req.TxCw.Timeout =
								(uint16_t) ((mcpsIndication->Buffer[1] << 8)
										| mcpsIndication->Buffer[2]);
						mlmeReq.Req.TxCw.Frequency =
								(uint32_t) ((mcpsIndication->Buffer[3] << 16)
										| (mcpsIndication->Buffer[4] << 8)
										| mcpsIndication->Buffer[5]) * 100;
						mlmeReq.Req.TxCw.Power = mcpsIndication->Buffer[6];
						LoRaMacMlmeRequest(&mlmeReq);
					}
					ComplianceTest.State = 1;
				}
					break;
				default:
					break;
				}
			}
			break;
		default:
			break;
		}
	}
// Switch LED 2 ON for each received downlink
}

/*!
 * \brief   MLME-Confirm event function
 *
 * \param   [IN] mlmeConfirm - Pointer to the confirm structure,
 *               containing confirm attributes.
 */
static void MlmeConfirm(MlmeConfirm_t *mlmeConfirm) {
	switch (mlmeConfirm->MlmeRequest) {
	case MLME_JOIN: {
		if (mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK) {
			// Status is OK, node has joined the network

			DeviceState = DEVICE_STATE_SEND;
		} else {
			// Join was not successful. Try to join again
			DeviceState = DEVICE_STATE_JOIN;
		}
		break;
	}
	case MLME_LINK_CHECK: {
		if (mlmeConfirm->Status == LORAMAC_EVENT_INFO_STATUS_OK) {
			// Check DemodMargin
			// Check NbGateways
			if (ComplianceTest.Running == true) {
				ComplianceTest.LinkCheck = true;
				ComplianceTest.DemodMargin = mlmeConfirm->DemodMargin;
				ComplianceTest.NbGateways = mlmeConfirm->NbGateways;
			}
		}
		break;
	}
	default:
		break;
	}
	NextTx = true;
}

/**
 *  application entry point.
 */
int main(void) {
	LoRaMacPrimitives_t LoRaMacPrimitives;
	LoRaMacCallback_t LoRaMacCallbacks;
	MibRequestConfirm_t mibReq;

	BoardInitMcu();
	BoardInitPeriph();

	GpioInit(&windSpeed, SPEED, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
//	GpioInit(&windVane, VANE, PIN_ANALOGIC, PIN_PUSH_PULL, PIN_PULL_UP, 0 );
	AdcInit(&windVane, VANE);


	FifoInit(&Uart1.FifoTx, txBuffer, 100);
	FifoInit(&Uart1.FifoRx, rxBuffer, 100);

	UartInit(&Uart1, 0, UART_TX, UART_RX);
	Uart1.IrqNotify = UartISR;

	UartConfig(&Uart1, RX_TX, 115200, UART_8_BIT, UART_1_STOP_BIT, NO_PARITY,
			NO_FLOW_CTRL);


	DeviceState = DEVICE_STATE_INIT;

	FifoFlush(&Uart1.FifoRx);
	FifoFlush(&Uart1.FifoTx);

	while (1) {
		Rotations = 0;
		GpioSetInterrupt(&windSpeed, IRQ_FALLING_EDGE, IRQ_MEDIUM_PRIORITY, irqRotation);
		DelayMs(5000);
		GpioRemoveInterrupt(&windSpeed);
		MPH = (Rotations * 1.492)/5;
		KmPH = (Rotations * 2.4)/5;

		vaneValue = AdcReadChannel(&windVane, 20);	// channel Number=20 for PB_14
		direction = directionMap(vaneValue, 0, 1023, 0, 360);
		calDirection = direction + offset;
		if(calDirection > 360)
			calDirection = calDirection - 360;
		if(calDirection < 0)
			calDirection = calDirection + 360;
//		windDirection = getHeading(calDirection);
//		strcpy(windDirection, getHeading(calDirection));
//		flag = 13;

	switch (DeviceState) {
	case DEVICE_STATE_INIT: {
		LoRaMacPrimitives.MacMcpsConfirm = McpsConfirm;
		LoRaMacPrimitives.MacMcpsIndication = McpsIndication;
		LoRaMacPrimitives.MacMlmeConfirm = MlmeConfirm;
		LoRaMacCallbacks.GetBatteryLevel = BoardGetBatteryLevel;
		LoRaMacInitialization(&LoRaMacPrimitives, &LoRaMacCallbacks);

		TimerInit(&TxNextPacketTimer, OnTxNextPacketTimerEvent);

		mibReq.Type = MIB_ADR;
		mibReq.Param.AdrEnable = LORAWAN_ADR_ON;
		LoRaMacMibSetRequestConfirm(&mibReq);

		mibReq.Type = MIB_PUBLIC_NETWORK;
		mibReq.Param.EnablePublicNetwork = LORAWAN_PUBLIC_NETWORK;
		LoRaMacMibSetRequestConfirm(&mibReq);

#if defined( USE_BAND_868 )
		LoRaMacTestSetDutyCycleOn( LORAWAN_DUTYCYCLE_ON);

#if( USE_SEMTECH_DEFAULT_CHANNEL_LINEUP == 1 )
		LoRaMacChannelAdd(3, ( ChannelParams_t )LC4);
		LoRaMacChannelAdd(4, ( ChannelParams_t )LC5);
		LoRaMacChannelAdd(5, ( ChannelParams_t )LC6);
		LoRaMacChannelAdd(6, ( ChannelParams_t )LC7);
		LoRaMacChannelAdd(7, ( ChannelParams_t )LC8);
		LoRaMacChannelAdd(8, ( ChannelParams_t )LC9);
		LoRaMacChannelAdd(9, ( ChannelParams_t )LC10);

		mibReq.Type = MIB_RX2_DEFAULT_CHANNEL;
		mibReq.Param.Rx2DefaultChannel = ( Rx2ChannelParams_t ) { 869525000,
						DR_3 };
		LoRaMacMibSetRequestConfirm(&mibReq);

		mibReq.Type = MIB_RX2_CHANNEL;
		mibReq.Param.Rx2Channel =
				( Rx2ChannelParams_t ) { 869525000, DR_3 };
		LoRaMacMibSetRequestConfirm(&mibReq);
#endif

#endif
		mibReq.Type = MIB_DEVICE_CLASS;
		mibReq.Param.Class = CLASS_C;
		LoRaMacMibSetRequestConfirm(&mibReq);

		DeviceState = DEVICE_STATE_JOIN;
		break;
	}
	case DEVICE_STATE_JOIN: {
#if( OVER_THE_AIR_ACTIVATION != 0 )
		MlmeReq_t mlmeReq;

		// Initialize LoRaMac device unique ID
		// BoardGetUniqueId( DevEui );  // TTN Registration is through DevEUI in OTAA Mode

		mlmeReq.Type = MLME_JOIN;

		mlmeReq.Req.Join.DevEui = DevEui;
		mlmeReq.Req.Join.AppEui = AppEui;
		mlmeReq.Req.Join.AppKey = AppKey;
		mlmeReq.Req.Join.NbTrials = 3;

		if (NextTx == true) {
			LoRaMacMlmeRequest(&mlmeReq);
		}
		DeviceState = DEVICE_STATE_SLEEP;
#else
		// Choose a random device address if not already defined in Commissioning.h
		if( DevAddr == 0 )
		{
			// Random seed initialization
			srand1( BoardGetRandomSeed( ) );

			// Choose a random device address
			DevAddr = randr( 0, 0x01FFFFFF );
		}

		mibReq.Type = MIB_NET_ID;
		mibReq.Param.NetID = LORAWAN_NETWORK_ID;
		LoRaMacMibSetRequestConfirm( &mibReq );

		mibReq.Type = MIB_DEV_ADDR;
		mibReq.Param.DevAddr = DevAddr;
		LoRaMacMibSetRequestConfirm( &mibReq );

		mibReq.Type = MIB_NWK_SKEY;
		mibReq.Param.NwkSKey = NwkSKey;
		LoRaMacMibSetRequestConfirm( &mibReq );

		mibReq.Type = MIB_APP_SKEY;
		mibReq.Param.AppSKey = AppSKey;
		LoRaMacMibSetRequestConfirm( &mibReq );

		mibReq.Type = MIB_NETWORK_JOINED;
		mibReq.Param.IsNetworkJoined = true;
		LoRaMacMibSetRequestConfirm( &mibReq );

		DeviceState = DEVICE_STATE_SEND;
#endif
		break;
	}
	case DEVICE_STATE_SEND: {

		if (NextTx == true) {
			PrepareTxFrame(AppPort);

			NextTx = SendFrame();
		}
		if (ComplianceTest.Running == true) {
			// Schedule next packet transmission
			TxDutyCycleTime = 5000; // 5000 ms
		} else {
			// Schedule next packet transmission
			TxDutyCycleTime = APP_TX_DUTYCYCLE
					+ randr(-APP_TX_DUTYCYCLE_RND, APP_TX_DUTYCYCLE_RND);
		}
		DeviceState = DEVICE_STATE_CYCLE;
		break;
	}
	case DEVICE_STATE_CYCLE: {
		DeviceState = DEVICE_STATE_SLEEP;

		// Schedule next packet transmission
		TimerSetValue(&TxNextPacketTimer, TxDutyCycleTime);
		TimerStart(&TxNextPacketTimer);
		break;
	}
	case DEVICE_STATE_SLEEP: {
		// Wake up through events
		TimerLowPowerHandler();
		break;
	}
	default: {
		DeviceState = DEVICE_STATE_INIT;
		break;
	}
	}
	}
}


void UartISR(UartNotifyId_t id) {

	if (id == UART_NOTIFY_RX) {
		flag = 1;
	}

	if (id == UART_NOTIFY_TX) {
		flag = 0;
//		UartReEnableRx(); // Implicit declaration warning can be ignored
	}
}

void irqRotation(){
	if ((HAL_GetTick() - ContactBounceTime) > 15 ) {
		Rotations++;
	    ContactBounceTime = HAL_GetTick();
	    txFlag = 1;
	}
}

uint16_t directionMap(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max)
		{
		  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
		}

char getHeading(uint16_t direction) {
//	char heading[3];
	if(direction < 22){
		strcpy(heading, "NOR");
		return heading;
		}
	else if (direction < 67){
		strcpy(heading, "N-E");
		return heading;
		}
	else if (direction < 112){
		strcpy(heading, "EAS");
		return heading;
		}
	else if (direction < 157){
		strcpy(heading, "S-E");
		return heading;
		}
	else if (direction < 212){
		strcpy(heading, "SOU");
		return heading;
		}
	else if (direction < 247){
		strcpy(heading, "S-W");
		return heading;
		}
	else if (direction < 292){
		strcpy(heading, "WES");
		return heading;
		}
	else if (direction < 337){
		strcpy(heading, "N-W");
		return heading;
		}
	else{
		strcpy(heading, "NOR");
		return heading;
		}
}


/***********************************************************************************************************************
 * This file was generated by the MCUXpresso Config Tools. Any manual edits made to this file
 * will be overwritten if the respective MCUXpresso Config Tools is used to update this file.
 **********************************************************************************************************************/

#include "usb_host_config.h"
#include "usb_host.h"
#include "fsl_device_registers.h"
#include "usb_host_hid.h"

#include "usb_host_interface_0_hid_keyboard.h"
#include "fsl_common.h"
#include "usb_host_app.h"

#if ((!USB_HOST_CONFIG_KHCI) && (!USB_HOST_CONFIG_EHCI) && (!USB_HOST_CONFIG_OHCI) && (!USB_HOST_CONFIG_IP3516HS))
#error Please enable USB_HOST_CONFIG_KHCI, USB_HOST_CONFIG_EHCI, USB_HOST_CONFIG_OHCI, or USB_HOST_CONFIG_IP3516HS in file usb_host_config.
#endif

#include "pin_mux.h"
#if ((defined(FSL_FEATURE_SOC_USBPHY_COUNT) && (FSL_FEATURE_SOC_USBPHY_COUNT > 0U)) && \
    ((defined(USB_HOST_CONFIG_EHCI) && (USB_HOST_CONFIG_EHCI > 0U)) || \
     (defined(USB_HOST_CONFIG_IP3516HS) && (USB_HOST_CONFIG_IP3516HS > 0U))))
#include "usb_phy.h"
#endif
#include "clock_config.h"

#if ((defined(USB_HOST_CONFIG_OHCI) && (USB_HOST_CONFIG_OHCI > 0U)) || \
      defined(USB_HOST_CONFIG_IP3516HS) && (USB_HOST_CONFIG_IP3516HS > 0U))
#include "fsl_power.h"
#endif
/*******************************************************************************
 * Definitions
 ******************************************************************************/
 
/* USB PHY configuration */
#ifndef BOARD_USB_PHY_D_CAL
#define BOARD_USB_PHY_D_CAL (0x0CU)
#endif
#ifndef BOARD_USB_PHY_TXCAL45DP
#define BOARD_USB_PHY_TXCAL45DP (0x06U)
#endif
#ifndef BOARD_USB_PHY_TXCAL45DM
#define BOARD_USB_PHY_TXCAL45DM (0x06U)
#endif
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*!
 * @brief host callback function.
 *
 * device attach/detach callback function.
 *
 * @param deviceHandle          device handle.
 * @param configurationHandle   attached device configuration descriptor information.
 * @param eventCode             callback event code, please reference to enumeration host_event_t.
 *
 * @retval kStatus_USB_Success              The host is initialized successfully.
 * @retval kStatus_USB_NotSupported         The application doesn't support the configuration.
 */
static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode);

void USB_HostClockInit(void);
void USB_HostIsrEnable(void);
void USB_HostTaskFn(void *hostHandle);

/*******************************************************************************
 * Variables
 ******************************************************************************/

usb_host_handle g_HostHandle;

/*******************************************************************************
 * Code
 ******************************************************************************/

 /*!
 * @brief MRT0 interrupt service routine
 */
void USB_OTG2_IRQHandler(void)
{
    USB_HostEhciIsrFunction(g_HostHandle);
}

/*!
 * @brief Initializes USB specific setting that was not set by the Clocks tool.
 */
void USB_HostClockInit(void)
{
    usb_phy_config_struct_t phyConfig = {
        BOARD_USB_PHY_D_CAL, BOARD_USB_PHY_TXCAL45DP, BOARD_USB_PHY_TXCAL45DM,
    };
    uint32_t notUsed = 0;
	
    if (USB_HOST_CONTROLLER_ID == kUSB_ControllerEhci0)
    {
        CLOCK_EnableUsbhs0PhyPllClock(kCLOCK_Usbphy480M, 480000000U);
        CLOCK_EnableUsbhs0Clock(kCLOCK_Usb480M, 480000000U);
    }
    else
    {
        CLOCK_EnableUsbhs1PhyPllClock(kCLOCK_Usbphy480M, 480000000U);
        CLOCK_EnableUsbhs1Clock(kCLOCK_Usb480M, 480000000U);
    }

    USB_EhciPhyInit(USB_HOST_CONTROLLER_ID, notUsed, &phyConfig);
}

/*!
 * @brief Enables interrupt service routines for device.
 */
void USB_HostIsrEnable(void)
{
    uint8_t irqNumber;
#if ((defined USB_HOST_CONFIG_KHCI) && (USB_HOST_CONFIG_KHCI > 0U))
    IRQn_Type usbHOSTKhciIrq[] = USB_IRQS;
    irqNumber = usbHOSTKhciIrq[USB_HOST_CONTROLLER_ID - kUSB_ControllerKhci0];
#endif
#if ((defined USB_HOST_CONFIG_EHCI) && (USB_HOST_CONFIG_EHCI > 0U))
    IRQn_Type usbHOSTEhciIrq[] = USBHS_IRQS;
    irqNumber = usbHOSTEhciIrq[USB_HOST_CONTROLLER_ID - kUSB_ControllerEhci0];
#endif
#if ((defined USB_HOST_CONFIG_IP3516HS) && (USB_HOST_CONFIG_IP3516HS > 0U))
    IRQn_Type usbHsIrqs[] = USBHSH_IRQS;
    irqNumber = usbHsIrqs[USB_HOST_CONTROLLER_ID - kUSB_ControllerIp3516Hs0];
#endif
#if ((defined USB_HOST_CONFIG_OHCI) && (USB_HOST_CONFIG_OHCI > 0U))
    IRQn_Type usbHsIrqs[] = USBFSH_IRQS;
    irqNumber = usbHsIrqs[USB_HOST_CONTROLLER_ID - kUSB_ControllerOhci0];
#endif

/* Install isr, set priority, and enable IRQ. */
#if defined(__GIC_PRIO_BITS)
    GIC_SetPriority((IRQn_Type)irqNumber, USB_HOST_INTERRUPT_PRIORITY);
#else
    NVIC_SetPriority((IRQn_Type)irqNumber, USB_HOST_INTERRUPT_PRIORITY);
#endif
    EnableIRQ((IRQn_Type)irqNumber);
}

/*!
 * @brief USB host task. This function should be called periodically.
 *
 * @param *hostHandle Pointer to host handle.
 */
void USB_HostTaskFn(void *hostHandle)
{
#if defined(USB_HOST_CONFIG_KHCI) && (USB_HOST_CONFIG_KHCI > 0U)
    USB_HostKhciTaskFunction(hostHandle);
#endif
#if defined(USB_HOST_CONFIG_EHCI) && (USB_HOST_CONFIG_EHCI > 0U)
    USB_HostEhciTaskFunction(hostHandle);
#endif
#if ((defined USB_HOST_CONFIG_OHCI) && (USB_HOST_CONFIG_OHCI > 0U))
    USB_HostOhciTaskFunction(hostHandle);
#endif
#if ((defined USB_HOST_CONFIG_IP3516HS) && (USB_HOST_CONFIG_IP3516HS > 0U))
    USB_HostIp3516HsTaskFunction(hostHandle);
#endif
}

/*!
 * @brief USB isr function.
 */
static usb_status_t USB_HostEvent(usb_device_handle deviceHandle,
                                  usb_host_configuration_handle configurationHandle,
                                  uint32_t eventCode)
{
    usb_status_t status0;
    usb_status_t status = kStatus_USB_Success;
    /* Used to prevent from multiple processing of one interface;
     * e.g. when class/subclass/protocol is the same then one interface on a device is processed only by one interface on host */
    uint8_t processedInterfaces[USB_HOST_CONFIG_CONFIGURATION_MAX_INTERFACE] = {0};

    switch (eventCode & 0x0000FFFFU)
    {
        case kUSB_HostEventAttach:
            status0 = USB_HostInterface0HidKeyboardEvent(deviceHandle, configurationHandle, eventCode, processedInterfaces);
            if (status0 == kStatus_USB_NotSupported)
            {
                status = kStatus_USB_NotSupported;
            }
            break;

        case kUSB_HostEventNotSupported:
            usb_echo("Device not supported.\r\n");
            break;

        case kUSB_HostEventEnumerationDone:
            status0 = USB_HostInterface0HidKeyboardEvent(deviceHandle, configurationHandle, eventCode, processedInterfaces);
            if (status0 != kStatus_USB_Success)
            {
                status = kStatus_USB_Error;
            }
            break;

        case kUSB_HostEventDetach:
            status0 = USB_HostInterface0HidKeyboardEvent(deviceHandle, configurationHandle, eventCode, processedInterfaces);
            if (status0 != kStatus_USB_Success)
            {
                status = kStatus_USB_Error;
            }
            break;

        case kUSB_HostEventEnumerationFail:
            usb_echo("Enumeration failed\r\n");
            break;

        default:
            break;
    }
    return status;
}

/*!
 * @brief Completely initializes USB host.
 *
 * This function calls other USB host functions and directly initializes following: USB specific clocks, USB stack and host isr.
 *
 * @retval kStatus_USB_Success              The host is initialized successfully.
 * @retval kStatus_USB_InvalidHandle        The hostHandle is a NULL pointer.
 * @retval kStatus_USB_ControllerNotFound   Cannot find the controller according to the controller ID.
 * @retval kStatus_USB_AllocFail            Allocation memory fail.
 * @retval kStatus_USB_Error                Host mutex create fail; KHCI/EHCI mutex or KHCI/EHCI event create fail,
 *                                          or, KHCI/EHCI IP initialize fail.
 */
usb_status_t USB_HostApplicationInit(void)
{
    usb_status_t status;

    USB_HostClockInit();

    status = USB_HostInit(USB_HOST_CONTROLLER_ID, &g_HostHandle, USB_HostEvent);
    if (status != kStatus_USB_Success)
    {
        return status;
    } else {
        USB_HostInterface0HidKeyboardInit();
    }
    USB_HostIsrEnable();

    return status;
}

/*!
 * @brief USB host tasks function.
 *
 * This function runs the tasks for USB host.
 *
 * @return None.
 */
void USB_HostTasks(void)
{
    USB_HostTaskFn(g_HostHandle);
    USB_HostInterface0HidKeyboardTask();
}

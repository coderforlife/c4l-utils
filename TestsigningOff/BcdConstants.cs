// All enums are copied from http://msdn.microsoft.com/en-us/library/cc441427%28VS.85%29.aspx
// The GUIDs were found at http://technet.microsoft.com/en-us/magazine/2008.07.heyscriptingguy.aspx

using System;
using System.Collections.Generic;
using System.Text;

namespace BcdConstants
{
    class Guids
    {
        public const string SystemStore = "{9dea862c-5cdd-4e70-acc1-f32b344d4795}";
        public const string Current = "{fa926493-6f1c-4193-a414-58f0b2456d1e}";
    }

    enum BcdBootMgrElementTypes : uint
    {
        ObjectList_DisplayOrder        = 0x24000001,
        ObjectList_BootSequence        = 0x24000002,
        Object_DefaultObject           = 0x23000003,
        Integer_Timeout                = 0x25000004,
        Boolean_AttemptResume          = 0x26000005,
        Object_ResumeObject            = 0x23000006,
        ObjectList_ToolsDisplayOrder   = 0x24000010,
        Device_BcdDevice               = 0x21000022,
        String_BcdFilePath             = 0x22000023,
    }

    enum BcdDeviceObjectElementTypes : uint
    {
        Integer_RamdiskImageOffset   = 0x35000001,
        Integer_TftpClientPort       = 0x35000002,
        Integer_SdiDevice            = 0x31000003,
        Integer_SdiPath              = 0x32000004,
        Integer_RamdiskImageLength   = 0x35000005,
    }

    enum BcdLibraryElementTypes : uint
    {
        Device_ApplicationDevice                   = 0x11000001,
        String_ApplicationPath                     = 0x12000002,
        String_Description                         = 0x12000004,
        String_PreferredLocale                     = 0x12000005,
        ObjectList_InheritedObjects                = 0x14000006,
        Integer_TruncatePhysicalMemory             = 0x15000007,
        ObjectList_RecoverySequence                = 0x14000008,
        Boolean_AutoRecoveryEnabled                = 0x16000009,
        IntegerList_BadMemoryList                  = 0x1700000a,
        Boolean_AllowBadMemoryAccess               = 0x1600000b,
        Integer_FirstMegabytePolicy                = 0x1500000c,
        Boolean_DebuggerEnabled                    = 0x16000010,
        Integer_DebuggerType                       = 0x15000011,
        Integer_SerialDebuggerPortAddress          = 0x15000012,
        Integer_SerialDebuggerPort                 = 0x15000013,
        Integer_SerialDebuggerBaudRate             = 0x15000014,
        Integer_1394DebuggerChannel                = 0x15000015,
        String_UsbDebuggerTargetName               = 0x12000016,
        Boolean_DebuggerIgnoreUsermodeExceptions   = 0x16000017,
        Integer_DebuggerStartPolicy                = 0x15000018,
        Boolean_EmsEnabled                         = 0x16000020,
        Integer_EmsPort                            = 0x15000022,
        Integer_EmsBaudRate                        = 0x15000023,
        String_LoadOptionsString                   = 0x12000030,
        Boolean_DisplayAdvancedOptions             = 0x16000040,
        Boolean_DisplayOptionsEdit                 = 0x16000041,
        Boolean_GraphicsModeDisabled               = 0x16000046,
        Integer_ConfigAccessPolicy                 = 0x15000047,
        Boolean_DisableIntegrityChecks			 = 0x16000048,
        Boolean_AllowPrereleaseSignatures          = 0x16000049,
    }

    enum BcdMemDiagElementTypes : uint
    {
        Integer_PassCount      = 0x25000001,
        Integer_FailureCount   = 0x25000003,
    }

    enum BcdOSLoaderElementTypes : uint
    {
        Device_OSDevice                       = 0x21000001,
        String_SystemRoot                     = 0x22000002,
        Object_AssociatedResumeObject         = 0x23000003,
        Boolean_DetectKernelAndHal            = 0x26000010,
        String_KernelPath                     = 0x22000011,
        String_HalPath                        = 0x22000012,
        String_DbgTransportPath               = 0x22000013,
        Integer_NxPolicy                      = 0x25000020,
        Integer_PAEPolicy                     = 0x25000021,
        Boolean_WinPEMode                     = 0x26000022,
        Boolean_DisableCrashAutoReboot        = 0x26000024,
        Boolean_UseLastGoodSettings           = 0x26000025,
        Boolean_AllowPrereleaseSignatures     = 0x26000027,
        Boolean_NoLowMemory                   = 0x26000030,
        Integer_RemoveMemory                  = 0x25000031,
        Integer_IncreaseUserVa                = 0x25000032,
        Boolean_UseVgaDriver                  = 0x26000040,
        Boolean_DisableBootDisplay            = 0x26000041,
        Boolean_DisableVesaBios               = 0x26000042,
        Integer_ClusterModeAddressing         = 0x25000050,
        Boolean_UsePhysicalDestination        = 0x26000051,
        Integer_RestrictApicCluster           = 0x25000052,
        Boolean_UseBootProcessorOnly          = 0x26000060,
        Integer_NumberOfProcessors            = 0x25000061,
        Boolean_ForceMaximumProcessors        = 0x26000062,
        Boolean_ProcessorConfigurationFlags   = 0x25000063,
        Integer_UseFirmwarePciSettings        = 0x26000070,
        Integer_MsiPolicy                     = 0x26000071,
        Integer_SafeBoot                      = 0x25000080,
        Boolean_SafeBootAlternateShell        = 0x26000081,
        Boolean_BootLogInitialization         = 0x26000090,
        Boolean_VerboseObjectLoadMode         = 0x26000091,
        Boolean_KernelDebuggerEnabled         = 0x260000a0,
        Boolean_DebuggerHalBreakpoint         = 0x260000a1,
        Boolean_EmsEnabled                    = 0x260000b0,
        Integer_DriverLoadFailurePolicy       = 0x250000c1,
        Integer_BootStatusPolicy              = 0x250000E0,
    }

    enum BcdLibrary_DebuggerType
    {
        Serial   = 0,
        IEEE1394 = 1,
        Usb      = 2,
    }

    enum BcdLibrary_SafeBoot
    {
        Minimal    = 0,
        Network    = 1,
        DsRepair   = 2,
    }

    enum BcdOSLoader_NxPolicy
    {
        OptIn       = 0,
        OptOut      = 1,
        AlwaysOff   = 2,
        AlwaysOn    = 3,
    }

    enum BcdOSLoader_PAEPolicy
    {
        Default        = 0,
        ForceEnable    = 1,
        ForceDisable   = 2,
    }

}
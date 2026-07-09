// Static import-plugin registry for the embedded PlayerPRO build used by
// chipmachine. The stock UNIX backend (Lin-PlugImport.c) discovers format
// loaders by scanning a directory for ".so" bundles and dlopen()ing them. We
// don't ship loose bundles; instead the format loaders we need are compiled
// straight into the library and registered here at MADInitLibrary() time. The
// generic dispatch helpers (PPImportFile/PPTestFile/...) are copied verbatim
// from Lin-PlugImport.c -- only MInitImportPlug/CloseImportPlug change.
//
// Public domain, like the rest of PlayerPRO (Antoine Rosset).

#include "RDriver.h"
#include "MADFileUtils.h"
#include "MADPrivate.h"
#include "embeddedPlugs.h"

#define CharlMADcheckLength 10

// Table of statically-linked import loaders. Each entry is one of the embedded
// mainXXX() functions plus the 4-char file type it claims. The native "MADK"
// format is handled directly by the driver (CheckMADFile / MADReadMAD), so it
// is not listed here. We register the older MADF/MADG family (the bulk of the
// Modland PlayerPRO directory) via mainMADfg.
typedef struct {
    MADPLUGFUNC func;
    const char* type;
} EmbeddedPlug;

static const EmbeddedPlug kEmbeddedPlugs[] = {
    {mainMADfg, "MADF"}, // also handles 'MADG'
};

#define NUM_EMBEDDED_PLUGS (sizeof(kEmbeddedPlugs) / sizeof(kEmbeddedPlugs[0]))

MADErr PPMADInfoFile(const char *AlienFile, MADInfoRec *InfoRec)
{
    MADSpec *theMAD;
    long     fileSize;
    UNFILE   fileID;
    MADErr   MADCheck;

    if ((MADCheck = CheckMADFile(AlienFile)) != MADNoErr) {
        return MADCheck;
    }

    theMAD = (MADSpec*)malloc(sizeof(MADSpec) + 200);

    fileID = iFileOpenRead(AlienFile);
    if (!fileID) {
        free(theMAD);
        return MADReadingErr;
    }
    fileSize = iGetEOF(fileID);

    iRead(sizeof(MADSpec), theMAD, fileID);
    iClose(fileID);

    strcpy(InfoRec->internalFileName, theMAD->name);

    InfoRec->totalPatterns = theMAD->numPat;
    InfoRec->partitionLength = theMAD->numPointers;
    InfoRec->totalTracks = theMAD->numChn;
    InfoRec->signature = 'MADK';
    strcpy(InfoRec->formatDescription, "MADK");
    InfoRec->totalInstruments = theMAD->numInstru;
    InfoRec->fileSize = fileSize;

    free(theMAD);
    theMAD = NULL;

    return MADNoErr;
}

MADErr CallImportPlug(MADLibrary *inMADDriver, int PlugNo, MADFourChar order,
                      char *AlienFile, MADMusic *theNewMAD, MADInfoRec *info)
{
    MADDriverSettings driverSettings = {0};
    return (*inMADDriver->ThePlug[PlugNo].IOPlug)(order, AlienFile, theNewMAD,
                                                  info, &driverSettings);
}

void MInitImportPlug(MADLibrary *inMADDriver, const char *PlugsFolderName)
{
    size_t i;
    (void)PlugsFolderName; // no on-disk scan: loaders are compiled in
    inMADDriver->ThePlug = (PlugInfo*)calloc(MAXPLUG, sizeof(PlugInfo));
    inMADDriver->TotalPlug = 0;
    for (i = 0; i < NUM_EMBEDDED_PLUGS && inMADDriver->TotalPlug < MAXPLUG; i++) {
        PlugInfo* p = &inMADDriver->ThePlug[inMADDriver->TotalPlug];
        p->IOPlug = kEmbeddedPlugs[i].func;
        p->mode = MADPlugImport;
        strncpy(p->type, kEmbeddedPlugs[i].type, sizeof(p->type) - 1);
        inMADDriver->TotalPlug++;
    }
}

void CloseImportPlug(MADLibrary *inMADDriver)
{
    free(inMADDriver->ThePlug);
    inMADDriver->ThePlug = NULL;
}

MADErr PPInfoFile(MADLibrary *inMADDriver, char *kindFile, char *AlienFile,
                  MADInfoRec *InfoRec)
{
    int      i;
    MADMusic aMAD;

    if (!strcmp(kindFile, "MADK")) {
        return PPMADInfoFile(AlienFile, InfoRec);
    }
    for (i = 0; i < inMADDriver->TotalPlug; i++) {
        if (!strcmp(kindFile, inMADDriver->ThePlug[i].type)) {
            return CallImportPlug(inMADDriver, i, MADPlugInfo, AlienFile, &aMAD,
                                  InfoRec);
        }
    }
    return MADCannotFindPlug;
}

MADErr PPImportFile(MADLibrary *inMADDriver, char *kindFile, char *AlienFile,
                    MADMusic **theNewMAD)
{
    int        i;
    MADInfoRec InfoRec;

    for (i = 0; i < inMADDriver->TotalPlug; i++) {
        if (!strcmp(kindFile, inMADDriver->ThePlug[i].type)) {
            *theNewMAD = (MADMusic*)calloc(sizeof(MADMusic), 1);
            if (!*theNewMAD) {
                return MADNeedMemory;
            }
            return CallImportPlug(inMADDriver, i, MADPlugImport, AlienFile,
                                  *theNewMAD, &InfoRec);
        }
    }
    return MADCannotFindPlug;
}

MADErr CheckMADFile(const char* name)
{
    UNFILE refNum;
    char   charl[CharlMADcheckLength];
    MADErr err;

    refNum = iFileOpenRead(name);
    if (!refNum) {
        return MADReadingErr;
    }
    iRead(CharlMADcheckLength, charl, refNum);
    if (charl[0] == 'M' && charl[1] == 'A' && charl[2] == 'D' &&
        charl[3] == 'K') {
        err = MADNoErr;
    } else {
        err = MADFileNotSupportedByThisPlug;
    }
    iClose(refNum);
    return err;
}

MADErr PPIdentifyFile(MADLibrary *inMADDriver, char *type, char *AlienFile)
{
    UNFILE     refNum;
    int        i;
    MADInfoRec InfoRec;
    MADErr     iErr = MADNoErr;

    strcpy(type, "!!!!");

    refNum = iFileOpenRead(AlienFile);
    if (!refNum) {
        return MADReadingErr;
    }
    if (iGetEOF(refNum) < 100) {
        iErr = MADIncompatibleFile;
    }
    iClose(refNum);
    if (iErr) {
        return iErr;
    }

    iErr = CheckMADFile(AlienFile);
    if (iErr == MADNoErr) {
        strcpy(type, "MADK");
        return MADNoErr;
    }

    for (i = 0; i < inMADDriver->TotalPlug; i++) {
        if (CallImportPlug(inMADDriver, i, MADPlugTest, AlienFile, NULL,
                           &InfoRec) == MADNoErr) {
            strcpy(type, inMADDriver->ThePlug[i].type);
            return MADNoErr;
        }
    }

    strcpy(type, "!!!!");
    return MADCannotFindPlug;
}

bool MADPlugAvailable(const MADLibrary *inMADDriver, const char* kindFile)
{
    int i;
    if (!strcmp(kindFile, "MADK")) {
        return true;
    }
    for (i = 0; i < inMADDriver->TotalPlug; i++) {
        if (!strcmp(kindFile, inMADDriver->ThePlug[i].type)) {
            return true;
        }
    }
    return false;
}

MADErr PPExportFile(MADLibrary *inMADDriver, char *kindFile, char *AlienFile,
                    MADMusic *theNewMAD)
{
    int        i;
    MADInfoRec InfoRec;
    for (i = 0; i < inMADDriver->TotalPlug; i++) {
        if (!strcmp(kindFile, inMADDriver->ThePlug[i].type)) {
            return CallImportPlug(inMADDriver, i, MADPlugExport, AlienFile,
                                  theNewMAD, &InfoRec);
        }
    }
    return MADCannotFindPlug;
}

MADErr PPTestFile(MADLibrary *inMADDriver, char *kindFile, char *AlienFile)
{
    int        i;
    MADMusic   aMAD;
    MADInfoRec InfoRec;
    for (i = 0; i < inMADDriver->TotalPlug; i++) {
        if (!strcmp(kindFile, inMADDriver->ThePlug[i].type)) {
            return CallImportPlug(inMADDriver, i, MADPlugTest, AlienFile, &aMAD,
                                  &InfoRec);
        }
    }
    return MADCannotFindPlug;
}

MADFourChar GetPPPlugType(MADLibrary *inMADDriver, short ID, MADFourChar mode)
{
    int i, x;
    if (ID >= inMADDriver->TotalPlug) {
        MADDebugStr(__LINE__, __FILE__, "PP-Plug ERROR. ");
    }
    for (i = 0, x = 0; i < inMADDriver->TotalPlug; i++) {
        if (inMADDriver->ThePlug[i].mode == mode ||
            inMADDriver->ThePlug[i].mode == MADPlugImportExport) {
            if (ID == x) {
                short       xx;
                MADFourChar type = '    ';
                xx = strlen(inMADDriver->ThePlug[i].type);
                if (xx > 4) {
                    xx = 4;
                }
                memcpy(&type, inMADDriver->ThePlug[i].type, xx);
                MADBE32(&type);
                return type;
            }
            x++;
        }
    }
    MADDebugStr(__LINE__, __FILE__, "PP-Plug ERROR II.");
    return MADNoErr;
}

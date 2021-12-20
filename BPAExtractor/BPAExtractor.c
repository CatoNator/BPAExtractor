#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define FILECOUNT (0xFF)
#define FILENAMELENGTH (13)

struct BPAFile
{
    char Name[FILENAMELENGTH + 1];
    uint32_t Size;
    uint8_t* Data;
};

void DecryptFileName(char* Input)
{
    for (int i = 0; i < FILENAMELENGTH; i++)
    {
        if (Input[i] != 0x00)
        {
            char ind = Input[i] - (117 - 3 * i);

            Input[i] = ind;
        }
    }
}

void DecryptMusicFile(uint8_t* FileData, uint32_t FileSize)
{
    for (int i = 0; i < FileSize; i++)
    {
        uint8_t In = FileData[i];

        In = (uint8_t)(((In << (i % 7)) | (In >> (8 - (i % 7)))) & 0xFF);
        In = (uint8_t)((In - (0x6D + (i * 0x11))) & 0xFF);

        FileData[i] = In;
    }
}

void UnpackBPA(const char* SourceFile, const char* DestFolder)
{
    struct BPAFile Records[FILECOUNT];

    FILE* BPArchive = fopen(SourceFile, "rb+");

    //read filecount
    uint32_t Files = 0;
    fread(&Files, sizeof(uint32_t), 1, BPArchive);

    printf("Filecount %u\n", Files);

    //file table has a fixed size of 255 files; only the marked ones count
    for (uint32_t i = 0; i < Files; i++)
    {
        //read the name
        char NameData[FILENAMELENGTH + 1];
        fread(&NameData, sizeof(char), FILENAMELENGTH, BPArchive);
        NameData[FILENAMELENGTH] = 0x00; //null termination

        //decrypt file name
        DecryptFileName(&NameData);

        //file size
        uint32_t Size = 0;
        fread(&Size, sizeof(uint32_t), 1, BPArchive);

        printf("File %u: %s, size %u\n", i, NameData, Size);

        //store info
        memcpy(&(Records[i].Name), &NameData, FILENAMELENGTH + 1);
        Records[i].Size = Size;
    }

    //seek to the end of the file table
    fseek(BPArchive, 4 + (0xFF * 17), SEEK_SET);

    //Read the file data
    for (int i = 0; i < Files; i++)
    {
        printf("Reading file %i\n", i);
        
        uint8_t* Data = malloc(Records[i].Size);
        
        fread(Data, 1, Records[i].Size, BPArchive);

        //uncomment in case not a music file
        DecryptMusicFile(Data, Records[i].Size);

        Records[i].Data = Data;
    }

    fclose(BPArchive);

    for (int i = 0; i < Files; i++)
    {
        printf("writing %s\n", Records[i].Name);
        
        //destination filename
        char FullName[0xFF];
        memset(&FullName, 0, 0xFF);

        strcat(&FullName, DestFolder);
        strcat(&FullName, "\\");
        strcat(&FullName, &(Records[i].Name));
        
        FILE* DestFile = fopen(&FullName, "wb+");

        const char* Test = "Test";

        fwrite((Records[i].Data), 1, Records[i].Size, DestFile);

        fclose(DestFile);
    }
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        printf("BPAExtract - Extract Death Rally archives (*.BPA) into folders.\nUsage: BPAExtract <inputfile> <destfolder>\n");
        return 0;
    }

    //unpack the file
    UnpackBPA(argv[1], argv[2]);

    return 0;
}
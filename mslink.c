#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

/*
 * mslink: Allow to create Windows Shortcut without the need of Windows
 *
 * Copyright (C) 2019 Mikaël Le Bohec
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program. If not, see http://www.gnu.org/licenses/.
 */

/*
 * Cette application permet de créer un Raccourci Windows (Fichier .LNK)
 *
 * Application créée en se basant sur la doc
 *   http://msdn.microsoft.com/en-us/library/dd871305.aspx
 */

char *LNK_TARGET = NULL;
char *LNK_TARGET_ori = NULL;
char *OUTPUT_FILE = NULL;
char *param_HasName = NULL;
char *param_HasWorkingDir = NULL;
char *param_HasArguments = NULL;
char *param_HasIconLocation = NULL;
int IS_PRINTER_LINK = 0;
int IS_ROOT_LNK = 0;
int IS_NETWORK_LNK = 0;

int extension_strlen = 0;

char *TARGET_ROOT = NULL;
char *TARGET_ROOT_new = NULL;
char *TARGET_LEAF = NULL;

char *FileAttributes = NULL;
int FileAttributes_strlen = 0;

char *PREFIX_ROOT = NULL;
int PREFIX_ROOT_strlen = 0;

char *PREFIX_OF_TARGET = NULL;
int PREFIX_OF_TARGET_strlen = 0;

char Item_Data[18];
int Item_Data_strlen = 18;

char *TARGET_ROOT_fill_with_0 = NULL;
int TARGET_ROOT_fill_with_0_strlen;

/*
 * Variables issues de la documentation officielle de Microsoft
 */

/* HeaderSize */
char HeaderSize[] = "\x4c\x00\x00\x00";
int HeaderSize_strlen = 4;

/* LinkCLSID */
char LinkCLSID_ori[] = "00021401-0000-0000-c000-000000000046";
unsigned char LinkCLSID[16] = {0};
int LinkCLSID_strlen = 16;

/* LinkFlags */
int LinkFlags_1_strlen = 1;
int LinkFlags_1 = 0x00;
char LinkFlags_2_3_4[] = "\x01\x00\x00";
int LinkFlags_2_3_4_strlen = 3;

int HasLinkTargetIDList = 0x01;
int HasName = 0x04;
int HasWorkingDir = 0x10;
int HasArguments = 0x20;
int HasIconLocation = 0x40;

/* FILE_ATTRIBUTE_DIRECTORY */
char FileAttributes_Directory[] = "\x10\x00\x00\x00";
int FileAttributes_Directory_strlen = 4;

/* FILE_ATTRIBUTE_ARCHIVE */
char FileAttributes_File[] = "\x20\x00\x00\x00";
int FileAttributes_File_strlen = 4;

/* CreationTime */
char CreationTime[] = "\x00\x00\x00\x00\x00\x00\x00\x00";
int CreationTime_strlen = 8;

/* AccessTime */
char AccessTime[] = "\x00\x00\x00\x00\x00\x00\x00\x00";
int AccessTime_strlen = 8;

/* WriteTime */
char WriteTime[] = "\x00\x00\x00\x00\x00\x00\x00\x00";
int WriteTime_strlen = 8;

/* FileSize */
char FileSize[] = "\x00\x00\x00\x00";
int FileSize_strlen = 4;

/* IconIndex */
char IconIndex[] = "\x00\x00\x00\x00";
int IconIndex_strlen = 4;

/* ShowCommand : SW_SHOWNORMAL */
char ShowCommand[] = "\x01\x00\x00\x00";
int ShowCommand_strlen = 4;

/* Hotkey : No Hotkey */
char Hotkey[] = "\x00\x00";
int Hotkey_strlen = 2;

/* Reserved : Valeur non modifiable */
char Reserved[] = "\x00\x00";
int Reserved_strlen = 2;

/* Reserved2 : Valeur non modifiable */
char Reserved2[] = "\x00\x00\x00\x00";
int Reserved2_strlen = 4;

/* Reserved3 : Valeur non modifiable */
char Reserved3[] = "\x00\x00\x00\x00";
int Reserved3_strlen = 4;

/* TerminalID : Valeur non modifiable */
char TerminalID[] = "\x00\x00";
int TerminalID_strlen = 2;

/* CLSID_Computer */
char CLSID_Computer_ori[] = "20d04fe0-3aea-1069-a2d8-08002b30309d";
unsigned char CLSID_Computer[16] = {0};
int CLSID_Computer_strlen = 16;

/* CLSID_Network */
char CLSID_Network_ori[] = "208d2c60-3aea-1069-a2d7-08002b30309d";
unsigned char CLSID_Network[16] = {0};
int CLSID_Network_strlen = 16;

/*
 * Constantes trouvées à partir de l'analyse de fichiers lnk
 */

/* PREFIX_LOCAL_ROOT : Disque local */
char PREFIX_LOCAL_ROOT[] = "\x2f";
int PREFIX_LOCAL_ROOT_strlen = 1;

/* PREFIX_FOLDER : Dossier de fichiers */
char PREFIX_FOLDER[] = "\x31\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
int PREFIX_FOLDER_strlen = 12;

/* PREFIX_FILE : Fichier */
char PREFIX_FILE[] = "\x32\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
int PREFIX_FILE_strlen = 12;

/* PREFIX_NETWORK_ROOT : Racine de serveur de fichiers réseau */
char PREFIX_NETWORK_ROOT[] = "\xc3\x01\x81";
int PREFIX_NETWORK_ROOT_strlen = 3;

/* PREFIX_NETWORK_PRINTER : Imprimante réseau */
char PREFIX_NETWORK_PRINTER[] = "\xc3\x02\xc1";
int PREFIX_NETWORK_PRINTER_strlen = 3;

/* END_OF_STRING */
char END_OF_STRING[] = "\x00";
int END_OF_STRING_strlen = 1;

/***********************************************************************************
* Fonction permettant la conversion du format CLSID
************************************************************************************/
unsigned char charToHexDigit(char c)
{
	if (c >= 'A') {
		return (c - 'A' + 10);
	}
	else {
		return (c - '0');
	}
}

unsigned char twoCharsToByte(char c1, char c2)
{
	return charToHexDigit(toupper(c1)) * 16 + charToHexDigit(toupper(c2));
}

void convert_CLSID_to_DATA(char *src, unsigned char *dst) {
	dst[0] = twoCharsToByte(src[6], src[7]);
	dst[1] = twoCharsToByte(src[4], src[5]);
	dst[2] = twoCharsToByte(src[2], src[3]);
	dst[3] = twoCharsToByte(src[0], src[1]);
	dst[4] = twoCharsToByte(src[11], src[12]);
	dst[5] = twoCharsToByte(src[9], src[10]);
	dst[6] = twoCharsToByte(src[16], src[17]);
	dst[7] = twoCharsToByte(src[14], src[15]);
	dst[8] = twoCharsToByte(src[19], src[20]);
	dst[9] = twoCharsToByte(src[21], src[22]);
	dst[10] = twoCharsToByte(src[24], src[25]);
	dst[11] = twoCharsToByte(src[26], src[27]);
	dst[12] = twoCharsToByte(src[28], src[29]);
	dst[13] = twoCharsToByte(src[30], src[31]);
	dst[14] = twoCharsToByte(src[32], src[33]);
	dst[15] = twoCharsToByte(src[34], src[35]);
}
/***********************************************************************************/

void freeValue(char *myPointer) {
	if (myPointer != NULL) {
		free(myPointer);
		myPointer = NULL;
	}
}

void getParameters(int argc, char *argv[]) {
        int showHelp = 0;
        int i;
        int c;

	opterr = 0;

	while ((c = getopt (argc, argv, "hpl:o:n:w:a:i:")) != -1)
		switch (c)
		{
			case 'p':
				IS_PRINTER_LINK = 1;
				break;
			case 'l':
				LNK_TARGET = optarg;
				break;
			case 'o':
				OUTPUT_FILE = optarg;
				break;
			case 'n':
				param_HasName = optarg;
				break;
			case 'w':
				param_HasWorkingDir = optarg;
				break;
			case 'a':
				param_HasArguments = optarg;
				break;
			case 'i':
				param_HasIconLocation = optarg;
				break;
			case 'h':
				showHelp = 1;
				break;
			case '?':
				showHelp = 1;
				break;
			default:
				abort ();
		}

	for (i = optind; i < argc; i++) {
		showHelp = 1;
	}

	if (LNK_TARGET == NULL || OUTPUT_FILE == NULL || showHelp) {
		printf("# mslink v1.3 compiled at %s  %s\n"
		"# This application allows you to create a Windows Shortcut (.LNK file) without dependencies with Microsoft libraries.\n"
		"\n"
		"Usage :\n"
		"%s -l target_of_file_lnk [-n description] [-w working_dir] [-a cmd_args] [-i icon_path] -o my_filename.lnk [-p]\n"
		"\n"
		"Options :\n"
		" -l, --lnk-target               Specifies the target of the shortcut\n"
		" -o, --output-file              Saves the shortcut to a file\n"
		" -n, --name                     Specifies a description for the shortcut\n"
		" -w, --working-dir              Specifies the directory where the command will be launched\n"
		" -a, --arguments                Specifies the arguments of the launched command\n"
		" -i, --icon                     Specifies the path of the icon\n"
		" -p, --printer-link             Generates a network printer type shortcut\n", __DATE__, __TIME__, argv[0]);

		exit(1);
	}

	LNK_TARGET_ori = malloc(strlen(LNK_TARGET));
	if (LNK_TARGET_ori == NULL) exit(2);
	strcpy(LNK_TARGET_ori, LNK_TARGET);
}

void initVariables() {
	convert_CLSID_to_DATA(LinkCLSID_ori, LinkCLSID);
	convert_CLSID_to_DATA(CLSID_Computer_ori, CLSID_Computer);
	convert_CLSID_to_DATA(CLSID_Network_ori, CLSID_Network);
}

void detectTypeLnk() {
	/*
	 * On distingue si le lien est de type local ou réseau
	 * On définie la valeur Item_Data suivant le cas d'un lien réseau ou local
	 */
	int i;

	if (strncmp(LNK_TARGET, "\\\\", 2) == 0) {
		IS_NETWORK_LNK = 1;

		if (IS_PRINTER_LINK) {
			PREFIX_ROOT = PREFIX_NETWORK_PRINTER;
			PREFIX_ROOT_strlen = PREFIX_NETWORK_PRINTER_strlen;

			IS_ROOT_LNK = 1;
		}
		else {
			PREFIX_ROOT = PREFIX_NETWORK_ROOT;
			PREFIX_ROOT_strlen = PREFIX_NETWORK_ROOT_strlen;
		}

		Item_Data[0] = '\x1f';
		Item_Data[1] = '\x58';

		for (i = 0 ; i < CLSID_Network_strlen ; i++) {
			Item_Data[i+2] = CLSID_Network[i];
		}
	}
	else {
		PREFIX_ROOT = PREFIX_LOCAL_ROOT;
		PREFIX_ROOT_strlen = PREFIX_LOCAL_ROOT_strlen;

		Item_Data[0] = '\x1f';
		Item_Data[1] = '\x50';

		for (i = 0 ; i < CLSID_Computer_strlen ; i++) {
			Item_Data[i+2] = CLSID_Computer[i];
		}
	}
}

void splitTargetLnk() {
	/*
	 * On sépare le chemin racine du lien de la cible finale
	 */
	if (IS_ROOT_LNK) {
		TARGET_ROOT = LNK_TARGET;
	}
	else {
		if (IS_NETWORK_LNK) {
			TARGET_LEAF = strrchr(LNK_TARGET, '\\');
			if (TARGET_LEAF != NULL) {
				TARGET_LEAF[0] = '\0';
				TARGET_LEAF++;
			}
			TARGET_ROOT = LNK_TARGET;
		}
		else {
			TARGET_LEAF = strchr(LNK_TARGET, '\\');
			if (TARGET_LEAF != NULL) {
				TARGET_LEAF[0] = '\0';
				TARGET_LEAF++;
			}
			TARGET_ROOT_new = malloc(strlen(LNK_TARGET)+2);
			if (TARGET_ROOT_new == NULL) exit(2);
			strcpy(TARGET_ROOT_new, LNK_TARGET);
			TARGET_ROOT_new[strlen(LNK_TARGET)] = '\\';
			TARGET_ROOT = TARGET_ROOT_new;
		}
	}

	if (TARGET_LEAF != NULL && strlen(TARGET_LEAF) == 0) IS_ROOT_LNK = 1;
}

void selectPrefix() {
	/*
	 * On sélectionne le préfixe qui sera utilisé pour afficher l'icône du raccourci
	 */
	if (TARGET_LEAF != NULL) {
		char *extension;
		extension = strrchr(TARGET_LEAF, '.');
		if (extension != NULL) {
			extension++;
			extension_strlen = strlen(extension);
		}
	}

	PREFIX_OF_TARGET_strlen = 0;
	if (extension_strlen >= 1 && extension_strlen <= 3) {
		PREFIX_OF_TARGET = PREFIX_FILE;
		PREFIX_OF_TARGET_strlen = PREFIX_FILE_strlen;
		FileAttributes = FileAttributes_File;
		FileAttributes_strlen = FileAttributes_File_strlen;
	}
	else {
		PREFIX_OF_TARGET = PREFIX_FOLDER;
		PREFIX_OF_TARGET_strlen = PREFIX_FOLDER_strlen;
		FileAttributes = FileAttributes_Directory;
		FileAttributes_strlen = FileAttributes_Directory_strlen;
	}
}

/* Nécessaire à partir de Vista et supérieur sinon le lien est considéré comme vide (je n'ai trouvé nul part d'informations à ce sujet) */
void fillTarget_with_0() {
	TARGET_ROOT_fill_with_0 = calloc(strlen(TARGET_ROOT)+22, sizeof(char));
	if (TARGET_ROOT_fill_with_0 == NULL) exit(2);
	TARGET_ROOT_fill_with_0_strlen = strlen(TARGET_ROOT)+21;
	strcpy(TARGET_ROOT_fill_with_0, TARGET_ROOT);
}

void displayInfos() {
	/* Affiche une information sur la création du raccourci */
	printf("Création d'un raccourci de type \"");
	if (IS_PRINTER_LINK) {
		printf("imprimante");
	}
	else {
		if (extension_strlen >= 1 && extension_strlen <= 3) {
			printf("fichier");
		}
		else {
			printf("dossier");
		}
	}
	if (IS_NETWORK_LNK) {
		printf(" réseau");
	}
	else {
		printf(" local");
	}
	printf("\" avec pour cible %s %s\n", LNK_TARGET_ori, param_HasArguments == NULL ? "" : param_HasArguments);
}

void writeToFile() {
	/* On génère la sortie */

	FILE* fichier = NULL;
	int i;

	fichier = fopen(OUTPUT_FILE, "w");

	if (fichier != NULL) {
		for (i = 0; i < HeaderSize_strlen ; i++) { fprintf(fichier, "%c", HeaderSize[i]); }
		for (i = 0; i < LinkCLSID_strlen ; i++) { fprintf(fichier, "%c", LinkCLSID[i]); }
		if (param_HasName == NULL) {
			HasName = 0x00;
		}
		if (param_HasWorkingDir == NULL) {
			HasWorkingDir = 0x00;
		}
		if (param_HasArguments == NULL) {
			HasArguments = 0x00;
		}
		if (param_HasIconLocation == NULL) {
			HasIconLocation = 0x00;
		}
		LinkFlags_1 = HasLinkTargetIDList + HasName + HasWorkingDir + HasArguments + HasIconLocation;

		for (i = 0; i < LinkFlags_1_strlen ; i++) { fprintf(fichier, "%c", LinkFlags_1); }
		for (i = 0; i < LinkFlags_2_3_4_strlen ; i++) { fprintf(fichier, "%c", LinkFlags_2_3_4[i]); }
		for (i = 0; i < FileAttributes_strlen ; i++) { fprintf(fichier, "%c", FileAttributes[i]); }
		for (i = 0; i < CreationTime_strlen ; i++) { fprintf(fichier, "%c", CreationTime[i]); }
		for (i = 0; i < AccessTime_strlen ; i++) { fprintf(fichier, "%c", AccessTime[i]); }
		for (i = 0; i < WriteTime_strlen ; i++) { fprintf(fichier, "%c", WriteTime[i]); }
		for (i = 0; i < FileSize_strlen ; i++) { fprintf(fichier, "%c", FileSize[i]); }
		for (i = 0; i < IconIndex_strlen ; i++) { fprintf(fichier, "%c", IconIndex[i]); }
		for (i = 0; i < ShowCommand_strlen ; i++) { fprintf(fichier, "%c", ShowCommand[i]); }
		for (i = 0; i < Hotkey_strlen ; i++) { fprintf(fichier, "%c", Hotkey[i]); }
		for (i = 0; i < Reserved_strlen ; i++) { fprintf(fichier, "%c", Reserved[i]); }
		for (i = 0; i < Reserved2_strlen ; i++) { fprintf(fichier, "%c", Reserved2[i]); }
		for (i = 0; i < Reserved3_strlen ; i++) { fprintf(fichier, "%c", Reserved3[i]); }

		if (IS_ROOT_LNK) {
			int maTaille_Item_Data = Item_Data_strlen;
			int maTaille_IDLIST_ITEMS = PREFIX_ROOT_strlen + TARGET_ROOT_fill_with_0_strlen + END_OF_STRING_strlen;

			int maTaille_IDLIST = maTaille_Item_Data + 2 + maTaille_IDLIST_ITEMS + 2;
			unsigned char maTaille_IDLIST_hex[2] = {0};
			maTaille_IDLIST_hex[0] = (maTaille_IDLIST+2)%256;
			maTaille_IDLIST_hex[1] = (maTaille_IDLIST+2)/256;

			fprintf(fichier, "%c%c", maTaille_IDLIST_hex[0], maTaille_IDLIST_hex[1]);

			unsigned char maTaille_Item_Data_hex[2] = {0};
			maTaille_Item_Data_hex[0] = (maTaille_Item_Data+2)%256;
			maTaille_Item_Data_hex[1] = (maTaille_Item_Data+2)/256;

			fprintf(fichier, "%c%c", maTaille_Item_Data_hex[0], maTaille_Item_Data_hex[1]);
			for (i = 0; i < Item_Data_strlen ; i++) { fprintf(fichier, "%c", Item_Data[i]); }

			unsigned char maTaille_IDLIST_ITEMS_hex[2] = {0};
			maTaille_IDLIST_ITEMS_hex[0] = (maTaille_IDLIST_ITEMS+2)%256;
			maTaille_IDLIST_ITEMS_hex[1] = (maTaille_IDLIST_ITEMS+2)/256;

			fprintf(fichier, "%c%c", maTaille_IDLIST_ITEMS_hex[0], maTaille_IDLIST_ITEMS_hex[1]);
			for (i = 0; i < PREFIX_ROOT_strlen ; i++) { fprintf(fichier, "%c", PREFIX_ROOT[i]); }
			for (i = 0; i < TARGET_ROOT_fill_with_0_strlen ; i++) { fprintf(fichier, "%c", TARGET_ROOT_fill_with_0[i]); }
			for (i = 0; i < END_OF_STRING_strlen ; i++) { fprintf(fichier, "%c", END_OF_STRING[i]); }

		}
		else {
			int maTaille_Item_Data = Item_Data_strlen;
			int maTaille_IDLIST_ITEMS = PREFIX_ROOT_strlen + TARGET_ROOT_fill_with_0_strlen + END_OF_STRING_strlen;
			int maTaille_IDLIST_ITEMS_TARGET = PREFIX_OF_TARGET_strlen + strlen(TARGET_LEAF) + END_OF_STRING_strlen;

			int maTaille_IDLIST = maTaille_Item_Data + 2 + maTaille_IDLIST_ITEMS + 2 + maTaille_IDLIST_ITEMS_TARGET + 2;
			unsigned char maTaille_IDLIST_hex[2] = {0};
			maTaille_IDLIST_hex[0] = (maTaille_IDLIST+2)%256;
			maTaille_IDLIST_hex[1] = (maTaille_IDLIST+2)/256;

			fprintf(fichier, "%c%c", maTaille_IDLIST_hex[0], maTaille_IDLIST_hex[1]);

			unsigned char maTaille_Item_Data_hex[2] = {0};
			maTaille_Item_Data_hex[0] = (maTaille_Item_Data+2)%256;
			maTaille_Item_Data_hex[1] = (maTaille_Item_Data+2)/256;

			fprintf(fichier, "%c%c", maTaille_Item_Data_hex[0], maTaille_Item_Data_hex[1]);
			for (i = 0; i < Item_Data_strlen ; i++) { fprintf(fichier, "%c", Item_Data[i]); }

			unsigned char maTaille_IDLIST_ITEMS_hex[2] = {0};
			maTaille_IDLIST_ITEMS_hex[0] = (maTaille_IDLIST_ITEMS+2)%256;
			maTaille_IDLIST_ITEMS_hex[1] = (maTaille_IDLIST_ITEMS+2)/256;

			fprintf(fichier, "%c%c", maTaille_IDLIST_ITEMS_hex[0], maTaille_IDLIST_ITEMS_hex[1]);
			for (i = 0; i < PREFIX_ROOT_strlen ; i++) { fprintf(fichier, "%c", PREFIX_ROOT[i]); }
			for (i = 0; i < TARGET_ROOT_fill_with_0_strlen ; i++) { fprintf(fichier, "%c", TARGET_ROOT_fill_with_0[i]); }
			for (i = 0; i < END_OF_STRING_strlen ; i++) { fprintf(fichier, "%c", END_OF_STRING[i]); }

			unsigned char maTaille_IDLIST_ITEMS_TARGET_hex[2] = {0};
			maTaille_IDLIST_ITEMS_TARGET_hex[0] = (maTaille_IDLIST_ITEMS_TARGET+2)%256;
			maTaille_IDLIST_ITEMS_TARGET_hex[1] = (maTaille_IDLIST_ITEMS_TARGET+2)/256;

			fprintf(fichier, "%c%c", maTaille_IDLIST_ITEMS_TARGET_hex[0], maTaille_IDLIST_ITEMS_TARGET_hex[1]);
			for (i = 0; i < PREFIX_OF_TARGET_strlen ; i++) { fprintf(fichier, "%c", PREFIX_OF_TARGET[i]); }
			for (i = 0; i < strlen(TARGET_LEAF) ; i++) { fprintf(fichier, "%c", TARGET_LEAF[i]); }
			for (i = 0; i < END_OF_STRING_strlen ; i++) { fprintf(fichier, "%c", END_OF_STRING[i]); }
		}

		for (i = 0; i < TerminalID_strlen ; i++) { fprintf(fichier, "%c", TerminalID[i]); }

		if (param_HasName != NULL) {
			int maTaille_param_HasName = strlen(param_HasName);
			unsigned char maTaille_param_HasName_hex[2] = {0};
			maTaille_param_HasName_hex[0] = (maTaille_param_HasName)%256;
			maTaille_param_HasName_hex[1] = (maTaille_param_HasName)/256;

			fprintf(fichier, "%c%c", maTaille_param_HasName_hex[0], maTaille_param_HasName_hex[1]);
			for (i = 0; i < strlen(param_HasName) ; i++) { fprintf(fichier, "%c", param_HasName[i]); }
		}

		if (param_HasWorkingDir != NULL) {
			int maTaille_param_HasWorkingDir = strlen(param_HasWorkingDir);
			unsigned char maTaille_param_HasWorkingDir_hex[2] = {0};
			maTaille_param_HasWorkingDir_hex[0] = (maTaille_param_HasWorkingDir)%256;
			maTaille_param_HasWorkingDir_hex[1] = (maTaille_param_HasWorkingDir)/256;

			fprintf(fichier, "%c%c", maTaille_param_HasWorkingDir_hex[0], maTaille_param_HasWorkingDir_hex[1]);
			for (i = 0; i < strlen(param_HasWorkingDir) ; i++) { fprintf(fichier, "%c", param_HasWorkingDir[i]); }
		}

		if (param_HasArguments != NULL) {
			int maTaille_param_HasArguments = strlen(param_HasArguments);
			unsigned char maTaille_param_HasArguments_hex[2] = {0};
			maTaille_param_HasArguments_hex[0] = (maTaille_param_HasArguments)%256;
			maTaille_param_HasArguments_hex[1] = (maTaille_param_HasArguments)/256;

			fprintf(fichier, "%c%c", maTaille_param_HasArguments_hex[0], maTaille_param_HasArguments_hex[1]);
			for (i = 0; i < strlen(param_HasArguments) ; i++) { fprintf(fichier, "%c", param_HasArguments[i]); }
		}

		if (param_HasIconLocation != NULL) {
			int maTaille_param_HasIconLocation = strlen(param_HasIconLocation);
			unsigned char maTaille_param_HasIconLocation_hex[2] = {0};
			maTaille_param_HasIconLocation_hex[0] = (maTaille_param_HasIconLocation)%256;
			maTaille_param_HasIconLocation_hex[1] = (maTaille_param_HasIconLocation)/256;

			fprintf(fichier, "%c%c", maTaille_param_HasIconLocation_hex[0], maTaille_param_HasIconLocation_hex[1]);
			for (i = 0; i < strlen(param_HasIconLocation) ; i++) { fprintf(fichier, "%c", param_HasIconLocation[i]); }
		}

		fclose(fichier);
	}

}

int main(int argc, char *argv[])
{
	getParameters(argc, argv);

	initVariables();

	detectTypeLnk();

	splitTargetLnk();

	selectPrefix();

	fillTarget_with_0();

	displayInfos();

	writeToFile();

	freeValue(TARGET_ROOT_fill_with_0);
	freeValue(TARGET_ROOT_new);
	freeValue(LNK_TARGET_ori);

	return 0;
}

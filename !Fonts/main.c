/*
	!Fonts
	© Alex Waugh 1999

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

	$Log: main.c,v $
	Revision 1.1  2002/10/05 22:46:37  ajw
	Increase max groups to 100 and group name length to 256 (if filing system supports it)
	Fixed to work on select
	
	Revision 1.5  2000/01/02 13:32:38  AJW
	Fixed reading current Font$Path, which appears to have got broken when converting to Desk
	
	Revision 1.4  1999/12/29 13:48:14  AJW
	Removed Save automatically menu item as it doesn't work in RO4

	Revision 1.3  1999/11/21 00:29:44  AJW
	Added Desk_Error2 handling
	Removed Dialog functions to stop Null reason codes being claimed

	Revision 1.2  1999/11/19 23:17:14  AJW
	Changed to work with Desk

	Revision 1.1  1999/11/17 13:52:31  AJW
	Initial revision


*/

#include "Desk/WimpSWIs.h"
#include "Desk/KernelSWIs.h"
#include "Desk/Window.h"
#include "Desk/Error.h"
#include "Desk/Event.h"
#include "Desk/EventMsg.h"
#include "Desk/File.h"
#include "Desk/Handler.h"
#include "Desk/Hourglass.h"
#include "Desk/Icon.h"
#include "Desk/Menu.h"
#include "Desk/Msgs.h"
#include "Desk/Resource.h"
#include "Desk/Screen.h"
#include "Desk/Template.h"

#include "AJWLib/Menu.h"
#include "AJWLib/Window.h"
#include "AJWLib/Icon.h"
#include "AJWLib/Error2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define max_NUMBER_OF_GROUPS 100
#define VERSION "1.06 (5-Oct-02)"

#define menuitem_INFO 0
#define menuitem_HELP 1
#define menuitem_SAVECHOICES 2
#define menuitem_QUIT 3

#define mainmenuitem_DELETE 0
#define mainmenuitem_NEW 1


Desk_window_handle window,info;
char dummybuffer[1]="";
char newdir[256]="";
Desk_menu_ptr mainmenu,submenu,iconbarmenu;

struct groups
{
	char name[256];
};

struct groups directories[max_NUMBER_OF_GROUPS];
int numberofgroups;

Desk_window_handle deletewin;
int dirtodelete;
Desk_bool changed=Desk_FALSE;

Desk_bool CreateIcons(void)
{
	int i,icon;
	Desk_icon_createblock iconblock;
	for(i=0;i<max_NUMBER_OF_GROUPS;i++) {
		iconblock.window=window;
		/*Create Option icons*/
		iconblock.icondata.workarearect.min.x=24;
		iconblock.icondata.workarearect.max.x=68;
		iconblock.icondata.flags.value=0x1700B11B;
		iconblock.icondata.data.indirecttext.buffer=dummybuffer;
		iconblock.icondata.data.indirecttext.validstring="Soptoff,opton";
		iconblock.icondata.data.indirecttext.bufflen=1;
		iconblock.icondata.workarearect.min.y=-84*(i+1)+12;
		iconblock.icondata.workarearect.max.y=-28-84*i;
		Desk_Wimp_CreateIcon(&iconblock,&icon);
		/*Create directory icons*/
		iconblock.icondata.workarearect.min.x=92;
		iconblock.icondata.workarearect.max.x=160;
		iconblock.icondata.flags.value=0x1701501A;
		strcpy(iconblock.icondata.data.spritename,"directory");
		iconblock.icondata.workarearect.min.y=-84*(i+1);
		iconblock.icondata.workarearect.max.y=-16-(i*84);
		Desk_Wimp_CreateIcon(&iconblock,&icon);
		/*Create text icons*/
		iconblock.icondata.workarearect.min.x=184;
		iconblock.icondata.workarearect.max.x=424;
		iconblock.icondata.flags.value=0x17000111;
		iconblock.icondata.workarearect.min.y=-84*(i+1)+12;
		iconblock.icondata.workarearect.max.y=-28-84*i;
		iconblock.icondata.data.indirecttext.buffer=directories[i].name;
		iconblock.icondata.data.indirecttext.bufflen=256;
		iconblock.icondata.data.indirecttext.validstring=(char *)-1;
		Desk_Wimp_CreateIcon(&iconblock,&icon);
	}
	Desk_Icon_Select(window,0);
	Desk_Icon_Shade(window,0);
	return Desk_TRUE;
}

Desk_bool ReadDirs(void)
{
	char buffer[257];
	int next;
	numberofgroups=1;
	strcpy(directories[0].name,"ROM Fonts");
	Desk_Error2_CheckOS(Desk_SWI(7,5,Desk_SWI_OS_GBPB,9,"<Fonts$Dir>.Groups",buffer,1,0,256,NULL,NULL,NULL,NULL,NULL,&next));
	while (next!=-1 && numberofgroups<max_NUMBER_OF_GROUPS) {
		strcpy(directories[numberofgroups++].name,buffer);
		Desk_Error2_CheckOS(Desk_SWI(7,5,Desk_SWI_OS_GBPB,9,"<Fonts$Dir>.Groups",buffer,1,next,256,NULL,NULL,NULL,NULL,NULL,&next));
	}
	Desk_Window_SetExtent(window,0,-84*numberofgroups-16,448,0);
	Desk_Window_ForceRedraw(window,0,-99999,99999,0);
	return Desk_TRUE;
}

Desk_bool OptionClick(Desk_event_pollblock *block, void *r)
{
    char command[256];
	if (block->data.mouse.icon % 3 !=0) return(Desk_FALSE);
    if (block->data.mouse.button.data.menu==1) return(Desk_FALSE);
	if (Desk_Icon_GetSelect(window,block->data.mouse.icon)) strcpy(command,"FontInstall Fonts:"); else strcpy(command,"FontRemove Fonts:");
	strcat(command,directories[block->data.mouse.icon/3].name);
	strcat(command,".");
	Desk_Hourglass_On();
	Desk_Wimp_StartTask(command);
	Desk_Hourglass_Off();
	return Desk_TRUE;
}

Desk_bool MenuClick(Desk_event_pollblock *block, void *r)
{
	if (block->data.mouse.button.data.menu!=1) return Desk_FALSE;
	if (block->data.mouse.icon==-1) Desk_Menu_SetFlags(mainmenu,mainmenuitem_DELETE,Desk_FALSE,Desk_TRUE); else Desk_Menu_SetFlags(mainmenu,mainmenuitem_DELETE,Desk_FALSE,Desk_FALSE);
	if (block->data.mouse.icon<3) Desk_Menu_SetFlags(mainmenu,mainmenuitem_DELETE,Desk_FALSE,Desk_TRUE);
	dirtodelete=block->data.mouse.icon/3;
	if (dirtodelete>=numberofgroups) Desk_Menu_SetFlags(mainmenu,mainmenuitem_DELETE,Desk_FALSE,Desk_TRUE);
    Desk_Menu_Show(mainmenu,block->data.mouse.pos.x,block->data.mouse.pos.y);
	return Desk_TRUE;
}

Desk_bool DirClick(Desk_event_pollblock *block, void *r)
{
	char opendir[256];
	if (block->data.mouse.icon % 3 !=1) return Desk_FALSE;
    if (block->data.mouse.button.data.menu==1) return Desk_FALSE;
	Desk_Icon_Deselect(window,block->data.mouse.icon);
	if (block->data.mouse.icon==1) {
		Desk_Wimp_StartTask("Filer_OpenDir Resources:$.Fonts");
	} else {
		strcpy(opendir,"Filer_OpenDir Fonts:");
		strcat(opendir,directories[block->data.mouse.icon/3].name);
		Desk_Wimp_StartTask(opendir);
	}
	return Desk_TRUE;
}

Desk_bool Delete(Desk_event_pollblock *block,void *r)
{
	char wipe[256]="Wipe Fonts:";
	int i;
	Desk_Wimp_CreateMenu((Desk_menu_ptr)-1,0,0);
	if (numberofgroups<=1) return Desk_TRUE;
	strcat(wipe,directories[dirtodelete].name);
	strcat(wipe," ~CFR~V");
	for(i=dirtodelete;i<numberofgroups-1;i++) {
		strcpy(directories[i].name,directories[i+1].name);
		if (Desk_Icon_GetSelect(window,(i+1)*3)) Desk_Icon_Select(window,i*3); else Desk_Icon_Deselect(window,i*3);
	}
	numberofgroups--;
	Desk_Window_SetExtent(window,0,-84*numberofgroups-16,448,0);
	Desk_Window_ForceRedraw(window,0,-99999,99999,0);
	Desk_Wimp_StartTask(wipe);
	changed=Desk_TRUE;
	return Desk_TRUE;
}

void OpenDialog(void)
{
	Desk_window_state blk;
	char text[100]="Are you sure you want to delete directory '";
	strcat(text,directories[dirtodelete].name);
	strcat(text,"' and all the fonts in it?");
	Desk_Icon_SetText(deletewin,0,text);
	Desk_Wimp_GetWindowState(deletewin,&blk);
	Desk_Wimp_CreateMenu((Desk_menu_ptr)deletewin,(Desk_screen_size.x-blk.openblock.screenrect.max.x+blk.openblock.screenrect.min.x)/2,(Desk_screen_size.y+blk.openblock.screenrect.max.y-blk.openblock.screenrect.min.y)/2);
}

Desk_bool CloseDialog(Desk_event_pollblock *e,void *r)
{
	Desk_Wimp_CreateMenu((Desk_menu_ptr)-1,0,0);
	return Desk_TRUE;
}

Desk_bool CreateDir(void)
{
	char create[256]="Fonts:";
	int type;
	if (strcmp(newdir,"")==0) Desk_Error2_HandleText("There must be a name to give to the new directory");
	if (numberofgroups>=max_NUMBER_OF_GROUPS) Desk_Error2_HandleText("You cannot have any more directories");
	strcat(create,newdir);
    Desk_Error2_CheckOS(Desk_SWI(2,1,Desk_SWI_OS_File,17,create,&type));
    if (type!=0) Desk_Error2_HandleText("Directory already exists");
	Desk_Error2_CheckOS(Desk_SWI(5,0,Desk_SWI_OS_File,8,create,NULL,NULL,0));
	strcpy(directories[numberofgroups].name,newdir);
	Desk_Icon_Deselect(window,numberofgroups*3);
	numberofgroups++;
	Desk_Window_SetExtent(window,0,-84*numberofgroups-16,448,0);
	Desk_Window_ForceRedraw(window,0,-99999,99999,0);
	strcpy(newdir,"");
	changed=Desk_TRUE;
	return Desk_TRUE;
}

Desk_bool IconBarClick(Desk_event_pollblock *block, void *r)
{
	if (block->data.mouse.button.data.select==1) {
		Desk_Window_Show(window,Desk_open_CENTERED);
		return Desk_TRUE;
	}
	return Desk_FALSE;
}

Desk_bool Quit(Desk_message_block *block, void *r)
{
	if (changed) {
		Desk_Error_Report(1,"You have added or deleted a directory, but have not saved the choices.");
		if (block!=NULL) {
			block->header.yourref=block->header.myref;
			Desk_Wimp_SendMessage(Desk_event_USERMESSAGEACK,block,block->header.sender,0);
		}
	} else Desk_Event_CloseDown();
	return Desk_TRUE;
}

Desk_bool SaveChoices(void)
{
	FILE *choices;
	int i;
	if ((choices=fopen("<Fonts$Dir>.!Boot","w"))==NULL) Desk_Error2_HandleText("Unable to save choices");
	fprintf(choices,"|!Boot file for !Fonts\n\nSet Fonts$Dir <Obey$Dir>\nSet Fonts$Path <Fonts$Dir>.Groups.\n\nIconSprites <Fonts$Dir>.!Sprites\n\n");
	for(i=1;i<numberofgroups;i++) if (Desk_Icon_GetSelect(window,i*3)) fprintf(choices,"FontInstall Fonts:%s.\n",directories[i].name);
	fclose(choices);
	Desk_Wimp_StartTask("SetType <Fonts$Dir>.!Boot Obey");
	changed=Desk_FALSE;
	return Desk_TRUE;
}

void IconBarMenuSelection(int entry, void *r)
{
	switch (entry) {
	  case menuitem_HELP:
		Desk_Wimp_StartTask("Filer_Run <Fonts$Dir>.!Help");
		break;
	  case menuitem_SAVECHOICES:
		SaveChoices();
	  	break;
	  case menuitem_QUIT:
	  	Quit(NULL,NULL);
	  	break;
	}
}

void ReadFontPath(void)
{
	char buff[1024]="";
	char *path,*buffer=buff;
	int i;
	Desk_SWI(5,0,Desk_SWI_OS_ReadVarVal,"Font$Path",buffer,1024,0,0); /*Is Desk's implementation of this broken?*/
	while((path=strstr(buffer,"Fonts:"))!=NULL) {
		buffer=strtok(path+6,".");
		for(i=1;i<numberofgroups;i++) if (strcmp(buffer,directories[i].name)==0) Desk_Icon_Select(window,i*3);
		buffer+=strlen(buffer)+1;
	}
}

void MenuSelection(int entry, void *r)
{
	if (entry==mainmenuitem_DELETE) OpenDialog();
}

void NewMenuSelection(int entry, void *r)
{
	CreateDir();
}

int main(void)
{
	Desk_Error2_HandleAllSignals();
	Desk_Error2_SetHandler(AJWLib_Error2_ReportFatal);
	Desk_Resource_Initialise("Fonts");
	Desk_Event_Initialise("Fonts");
	Desk_EventMsg_Initialise();
	Desk_Screen_CacheModeInfo();
	Desk_EventMsg_Claim(Desk_message_MODECHANGE,Desk_event_ANY,Desk_Handler_ModeChange,NULL);
	Desk_EventMsg_Claim(Desk_message_PREQUIT,Desk_event_ANY,(Desk_event_handler)Quit,NULL);
	Desk_Template_Initialise();
	Desk_Template_LoadFile("Templates");
	Desk_Event_Claim(Desk_event_CLOSE,Desk_event_ANY,Desk_event_ANY,Desk_Handler_CloseWindow,NULL);
	Desk_Event_Claim(Desk_event_REDRAW,Desk_event_ANY,Desk_event_ANY,Desk_Handler_NullRedraw,NULL);
	Desk_Event_Claim(Desk_event_OPEN,Desk_event_ANY,Desk_event_ANY,Desk_Handler_OpenWindow,NULL);
	info=AJWLib_Window_CreateInfoWindow("Fonts","Font selection","© Alex Waugh 1998",VERSION);
	window=Desk_Window_Create("Main",Desk_template_TITLEMIN);
	deletewin=Desk_Window_Create("Delete",Desk_template_TITLEMIN);
	Desk_Event_Claim(Desk_event_CLICK,deletewin,1,CloseDialog,NULL);
	Desk_Event_Claim(Desk_event_CLICK,deletewin,2,Delete,NULL);
	iconbarmenu=AJWLib_Menu_Create("Fonts","Info,Help,Save choices,Quit",IconBarMenuSelection,NULL);
	Desk_Menu_AddSubMenu(iconbarmenu,menuitem_INFO,(Desk_menu_ptr)info);
	AJWLib_Menu_Attach(Desk_window_ICONBAR,Desk_event_ANY,iconbarmenu,Desk_button_MENU);
	mainmenu=AJWLib_Menu_Create("Fonts","Delete directory,New directory",MenuSelection,NULL);
    submenu=AJWLib_Menu_Create("Name:","",NewMenuSelection,NULL);
    Desk_Menu_MakeWritable(submenu,0,newdir,255,"AA-Za-z0-9");
    Desk_Menu_AddSubMenu(mainmenu,mainmenuitem_NEW,submenu);
	AJWLib_Icon_FullBarIcon("Fonts","!Fonts",-5,0x10000000);
	Desk_Event_Claim(Desk_event_CLICK,Desk_window_ICONBAR,Desk_event_ANY,IconBarClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,window,Desk_event_ANY,OptionClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,window,Desk_event_ANY,DirClick,NULL);
	Desk_Event_Claim(Desk_event_CLICK,window,Desk_event_ANY,MenuClick,NULL);
	CreateIcons();
	ReadDirs();
    ReadFontPath();
	while (Desk_TRUE) Desk_Event_Poll();
	return 0;
}


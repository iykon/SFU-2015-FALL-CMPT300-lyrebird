/*
 * Project:			Assignment #4: A Distributed Flock of Lyrebirds
 * Author:			Weida Pan
 * student nunmber:	301295773
 * SFU user name: 	lykonp
 * lecture section:	D1
 * instructor:		Brian G. Booth
 * TA:				Scott Kristjanson
 * date:			Dec. 1, 2015
 * */

/* 
 * file name: msgprototol.h	
 * description: this file defines all macros that are used to communicate
 * 				between lyrebird.server and lyrebird.client as protocal	
 * */
#ifndef MSGPROTOCOL_H
#define MSGPROTOCOL_H

#define LSWORK 0x0001 // server message telling client to work
#define LSDONE 0x0002 // server message telling client to exit
#define LCSUCC 0x0004 // client message telling server succeed to decrypt
#define LCFAIL 0x0008 // client message telling server fail to decrypt
#define LCREADY 0x0010// client message telling server ready for next task

#endif

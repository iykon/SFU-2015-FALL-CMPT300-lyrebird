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

#define LSWORK "s work"// server message telling client to work
#define LSDONE "s done"// server message telling client to exit
#define LCSUCC "c success"// client message telling server succeed to decrypt
#define LCFAIL "c failure"// client message telling server fail to decrypt
#define LCREADY "c ready"// client message telling server ready for next task

#endif

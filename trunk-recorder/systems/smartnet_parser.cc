#include "smartnet_parser.h"

using namespace std;
SmartnetParser::SmartnetParser() {
	lastaddress = 0;
	lastcmd = 0;
	numStacked = 0;
	numConsumed = 0;
}
double SmartnetParser::getfreq(int cmd) {

float freq;



/* Different Systems will have different band plans. Below is the one for WMATA which is a bit werid:*/
       if (cmd < 0x12e) {
                freq = float((cmd) * 0.025 + 489.0875);
        } else if (cmd < 0x2b0) {
                freq = float((cmd-380) * 0.025 + 489.0875);
        } else {
                freq = 0;
        }
//      cout << "LCMD: 0x" <<  hex << cmd << " Freq: " << freq << " Multi: " << (cmd - 308) * 0.025 << " CMD: " << dec << cmd << endl;
/*

	if (cmd < 0x1b8) {
		freq = float(cmd * 0.025 + 851.0125);
	} else if (cmd < 0x230) {
		freq = float(cmd * 0.025 + 851.0125 - 10.9875);
	} else {
		freq = 0;
	}
*/
	return freq*1000000;
}


void
SmartnetParser::print_osw(std::string s)
{
	std::vector<std::string> x;

	boost::split(x, s, boost::is_any_of(","), boost::token_compress_on);

	int full_address = atoi( x[0].c_str() );
	int status = full_address & 0x000F;
	long address = full_address & 0xFFF0;
	int groupflag = atoi( x[1].c_str() );
	int command = atoi( x[2].c_str() );

	struct osw_stru bosw;
	bosw.id = address;
	bosw.grp = groupflag;
	bosw.cmd = command;
	struct osw_stru* Inposw = &bosw;

	char  tempArea [512];
        unsigned short blockNum;
        char banktype;
        unsigned short tt1,tt2;
        static unsigned int ott1,ott2;


        if( Inposw->cmd == OSW_BACKGROUND_IDLE )
        {
                // idle word - normally ignore but may indicate transition into
                // un-numbered system or out of numbered system

								cout << "Background Idle" << endl;
                // returning here because idles sometimes break up a 3-OSW sequence!

                return;
        }

        if( Inposw->cmd >= OSW_AMSS_ID_MIN && Inposw->cmd <= OSW_AMSS_ID_MAX )
        {

                // on one system, the cell id also got stuck in the middle of the 3-word sequence!

                // Site ID
								cout << " Note Site: " << (unsigned short)(Inposw->cmd - OSW_AMSS_ID_MIN) << endl;

                // returning here because site IDs sometimes break up a 3-OSW sequence!

                return;
        }


        // maintain a sliding stack of 5 OSWs. If previous iteration used more than one,
        // don't utilize stack until all used ones have slid past.

        switch(numStacked) // note: drop-thru is intentional!
        {
        case 5:
        case 4:
                stack[4] = stack[3];
        case 3:
                stack[3] = stack[2];
        case 2:
                stack[2] = stack[1];
        case 1:
                stack[1] = stack[0];
        case 0:
                stack[0] = *Inposw;
                break;
        default:
                cout << "corrupt value for nstacked" << endl;
                break;
        }
        if(numStacked < 5)
        {
                ++numStacked;
        }
        if( numConsumed > 0)
        {
                if(--numConsumed > 0)
                {
                        return;
                }
        }
        if(  numStacked < 3 )
        {
                return; // at least need a window of 3 and 5 is better.
        }

        // look for some larger patterns first... parts of the sequences could be
        // mis-interpreted if taken out of context.
				cout << "[ 0cmd: " << getfreq(stack[0].cmd) << "\t0id: " << getfreq(stack[0].id) << "\t1cmd: " << getfreq(stack[1].cmd) << "\t1id: " << getfreq(stack[1].id) << "] \t0cmd: " << stack[0].cmd << "\t0id: "<< stack[0].id << "\t1cmd: " << stack[1].cmd << "\t1id: "<< stack[1].id << endl;

        if(stack[2].cmd == OSW_FIRST_NORMAL || stack[2].cmd == OSW_FIRST_TY2AS1)
        {
                switch(stack[1].cmd)
                {
        case OSW_EXTENDED_FCN:

                        if((stack[1].id & 0xfff0) == 0x26f0) { // ack type 2 text msg

                            cout << "ack type 2 text msg " <<  stack[2].id << " Msg: " << (stack[1].id & (unsigned short)15) << endl;

                        } else if((stack[1].id & 0xfff8) == 0x26E0) { // ack type 2 status

                                cout << "ack type 2 status " <<  stack[2].id << " Status " << (stack[1].id & (unsigned short)7) << endl;

                        } else if( (stack[0].id & 0xff00) == 0x1F00 &&
                           (stack[1].id & 0xfc00) == 0x2800 && stack[0].cmd == (stack[1].id & 0x3FF))
                        {
                                /* non-smartzone identity:  sysid[10]
                                   <sysid>              x   308
                                   <001010><ffffffffff> x   30b
                                   1Fxx                 x   <ffffffffff>
                                */
																cout << "non-smartzone identity, data chan: " << stack[0].cmd << endl;

                                numConsumed = 3; // we used up all 3
                                return;
                        } else if((stack[1].id & 0xfc00) == 0x2800)
                        {
                                /* smartzone identity: sysid[10]

                                      <sysid>              x   308
                                      <001010><ffffffffff> x   30b
                                */
																cout << "smartzone identity:" << stack[2].id << " data chan: " << (unsigned short)(stack[1].id & 0x03ff) << endl;

                        } else if((stack[1].id & 0xfc00) == 0x6000)
                        {
                                /* smartzone peer info:                         sysid[24]
                                                <sysid>              x   308
                                                <011000><ffffffffff> x   30b
                                */
                                // evidence of smartzone, but can't use this pair for id
                                //idlesWithNoId = 0; // got identifying info...
																cout << " smartzone peer info: " << endl;
                                /*if(mySys)
                                {
                                        if( mySys->isNetworkable() && !mySys->isNetworked()) {
                                                mySys->setNetworked();
                                        }
                                }*/
                        } else if((stack[1].id & 0xfc00) == 0x2400)
                        { // various
                                //idlesWithNoId = 0;

                                if( (stack[1].id & 0x03ff) == 0x21c)
                                {
																	cout << " affiliation: " << stack[2].id << endl;

                                 }
                        } else if( stack[1].id  == 0x2021) {
														cout << " mscancel: " << stack[2].id;
                            /*if(mySys) {
                                mySys->mscancel(stack[2].id);
                            }*/

                        } else {
                                //idlesWithNoId = 0;

         /* unknown sequence.
         example: <000000> fffa x 308 then 0321 x 30b gateway close?
         example: <001000> 3890 G 308      2021 G 30b
                  <001011> 811d I 308      2c4a I 30b
                  <001001> 83d3 I 308      261c I 30b

         known:   <011000>
                  <001010>


          */
                        }
                        numConsumed = 2;
                        return;

        case 0x320:
                        if(stack[0].cmd == OSW_EXTENDED_FCN)
                        {
                                // definitely smart-zone: 308 320 30b
                                //  <sysId>                             308
                                //  <cell id 6 bits><10 bits see below> 320
                                //      <011000><ffffffffff>            30b
                                //
                                /*
                                    10 bits:  bbb0v?AaES

                                        bbb - band: [800,?,800,821,900,?,?]
                                    32  v   - VOC
                                    8   A   - Astro
                                    4   a   - analog
                                    2   E   - 12kbit encryption
                                    1   S   - site is Active (if zero?)
                                */
                                numConsumed = 3;
                                //idlesWithNoId = 0; // got identifying info...
                                if( (stack[0].id & 0xfc00) == 0x6000)
                                {
																		cout << "something about a peer" << endl;
                                        /*if(mySys && (stack[2].id == mySys->sysId) )
                                        {
                                                mySys->note_neighbor((unsigned short)((stack[1].id>>10) & 0x3F),(unsigned short)(stack[0].id & 0x3ff));
                                                if( mySys->isNetworkable() && !mySys->isNetworked())
                                                {
                                                        mySys->setNetworked();
                                                }

                                                // if this describes 'us', note any astro or VOC characteristics
                                                // for alternate data channel, note that.

                                                if(mySys->isNetworked() && ((unsigned short)((stack[1].id>>10) & 0x3F) ==  mySys->siteId) ) {
                                                        if(stack[0].grp) { // if active data channel
                                                                if( stack[1].id & 8 ) {
                                                                        mySys->setSubType(" Astro");
                                                                }
                                                                if( stack[1].id & 32 ) {
                                                                        mySys->setOneChannel("/VOC");
                                                                }
                                                        } else {  // is an alternate data channel frequency
                                                                mySys->noteAltDchan((unsigned short)(stack[0].id & 0x3ff));
                                                        }
                                                }
                                        }*/
                                }
                                return;
                        }
                        break;

        case OSW_TY2_AFFILIATION:
                        // type II requires double word 308/310. single word version is type 1 status,
                        // whatever that means.
												cout << "OSW_TY2_AFFILIATION: " << stack[2].id <<  " and " << stack[1].id << endl;
												/*
                        if( mySys && Verbose)
                        {
                                mySys->note_affiliation(stack[2].id,stack[1].id);

                        }*/
                        break;

        case OSW_TY2_MESSAGE:
        //        sprintf(tempArea,"Radio %04x, Message %d [1..8]\n",stack[2].id,1+(stack[1].id&7));

												cout << "OSW_TY2_MESSAGE: " << stack[2].id << " MSG: " <<   (unsigned short)(stack[1].id&7) << endl;
                        /*if( mySys && Verbose)
                        {
                                mySys->note_text_msg(   stack[2].id,
                                                        "Msg",
                                                        (unsigned short)(stack[1].id&7)
                                                    );
                        }*/
                        break;

        case OSW_TY2_CALL_ALERT:
                        // type II call alert. we can ignore the 'ack', 0x31A
												cout << "OSW_TY2_CALL_ALERT" << endl;
                        /*if( mySys && Verbose)
                        {
                                mySys->note_page(stack[2].id, stack[1].id);
                        }*/
                        break;

        case OSW_SYSTEM_CLOCK:  // system clock

                                tt1 = stack[2].id;
                                tt2 = stack[1].id;
                                if( tt2 != ott2 || tt1 != ott1 )
                                {
                                        ott2 = tt2;
                                        ott1 = tt1;
                                        sprintf(tempArea,"%02u/%02u/%02u %02u:%02u",
                                                (tt1>>5)& 0x000f,
                                                (tt1)   & 0x001f,
                                                (tt1>>9),
                                                (tt2>>8)& 0x001f,
                                                (tt2)   & 0x00ff);
                                        cout << "OSW_SYSTEM_CLOCK: " << tempArea << endl;
                                }

                        break;

        case OSW_EMERG_ANNC:
												cout << "OSW_EMERG_ANNC: " << stack[2].id << endl;
                        /*if(mySys)
                        {
                               mySys->noteEmergencySignal(stack[2].id);
                        }*/
                        break;
        case OSW_AFFIL_FCN:
												cout << "OSW_AFFIL_FCN - ";

                                switch(stack[1].id & 0x000f)
                                {
                                case 0:
                                case 1:
                                case 2:
                                case 3:
                                case 4:
                                case 5:
                                case 6:
                                case 7:
																				cout << "Status: " << stack[2].id << " - " << (unsigned short)(stack[1].id & 0x0007) << endl;
                                        /* if(Verbose)
                                        {
                                                mySys->note_text_msg(   stack[2].id,
                                                                        "Status",
                                                                        (unsigned short)(stack[1].id & 0x0007)
                                                                );
                                        }*/
                                        break;
                                case 8:
																				cout << "Emergency Signal: " << stack[2].id << endl;
                                        //mySys->noteEmergencySignal(stack[2].id);
                                        break;
                                case 9: // ChangeMe request
                                case 10:// Ack Dynamic Group ID
                                case 11:// Ack Dynamic Multi ID
                                        break;
                                default:
                                        break;
                                }

                        break;

        // Type I Dynamic Regrouping ("patch")

        case 0x340: // size code A (motorola codes)
        case 0x341: // size code B
        case 0x342: // size code C
        case 0x343: // size code D
        case 0x344: // size code E
        case 0x345: // size code F
        case 0x346: // size code G
        case 0x347: // size code H
        case 0x348: // size code I
        case 0x349: // size code J
        case 0x34a: // size code K

                                // patch notification
                                // patch 'tag' is in stack[1].id
                                //group associated is stack[2].id

                                switch(stack[2].id & 7)
                                {
                                    case 0:  cout << " normal for type 1 groups ";
                                    case 3:  cout <<  "patch ";
                                    case 4:  cout << "emergency patch ";
																				cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " more: " << (char)('A'+(stack[1].cmd - 0x340)) << endl;

                                        break;
                                    case 5:
                                    case 7:
																				cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " more: " << (char)('A'+(stack[1].cmd - 0x340)) << endl;
                                        break;
                                    default:
                                        break;
                                }

                        break;
        case 0x34c: // size code M trunker = L

                                // patch notification
                                // patch 'tag' is in stack[1].id
                                //group associated is stack[2].id
                                switch(stack[2].id & 7)
                                {
                                    case 0: cout << " normal for type 1 groups ";
                                    case 3: cout <<  "patch ";
                                    case 4: cout << "emergency patch ";
																				cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " L" << endl;
																        break;
                                    case 5:
                                    case 7:
																				cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " L" << endl;
																	  		break;
                                    default:
                                        break;
                                }

                        break;
        case 0x34e: // size code O Trunker = M

                                // patch notification
                                // patch 'tag' is in stack[1].id
                                //group associated is stack[2].id
                                switch(stack[2].id & 7)
                                {
                                    case 0: cout << " normal for type 1 groups ";
                                    case 3: cout <<  "patch ";
                                    case 4: cout << "emergency patch ";


																					cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " M" << endl;
																	        break;
                                    case 5:
                                    case 7:
																				cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " M" << endl;

                                        break;
                                    default:
                                        break;
                                }

                        break;

        case 0x350: // size code Q Trunker = N

                                // patch notification
                                // patch 'tag' is in stack[1].id
                                //group associated is stack[2].id
                                switch(stack[2].id & 7)
                                {
                                    case 0: cout << " normal for type 1 groups ";
                                    case 3: cout <<  "patch ";
                                    case 4: cout << "emergency patch ";


																				cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " N" << endl;

                                        break;
                                    case 5:
                                    case 7:
																		cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " N" << endl;

                                        break;
                                    default:
                                        break;
                                }

                        break;
        default:
								cout << "default" << endl;
/*
                                if(stack[1].isFreq)
                                {
                                        if ( stack[2].cmd == OSW_FIRST_TY2AS1 && stack[1].grp)
                                        {
                                                blockNum = (unsigned short)((stack[1].id>>13)&7);
                                                banktype = mySys->types[blockNum];
                                                if( banktype == '2' )
                                                {
                                                        mySys->types[blockNum] = '?';
                                                        _settextposition(FBROW,1);
                                                        sprintf(tempArea,"Block %d <0..7> is Type IIi                         ",blockNum);
                                                        _outtext(tempArea);
                                                }
                                                mySys->setSubType("i");
                                        }
                                        if( stack[2].id != 0 )
                                        { // zero id used special way in hybrid system
                                                twoOSWcall();
                                        }
                                }*/

                        // otherwise 'busy', others...
                }

                numConsumed = 2;
                return;
        }

        // uhf/vhf equivalent of 308/320/30b

        if( stack[1].cmd == 0x0320 && stack[0].cmd == OSW_EXTENDED_FCN )
        {
                numConsumed = 3;
                //idlesWithNoId = 0; // got identifying info...
                if( (stack[0].id & 0xfc00) == 0x6000)
                {
										cout << "uhf/vhf equivalent of 308/320/30b" << endl;
										/*
                        if(mySys && (stack[2].id == mySys->sysId) )
                        {
                                if( myFreqMap->isFrequency((unsigned short)(stack[0].id&0x3ff),mySys->getBandPlan()) )
                                {
                                        mySys->note_neighbor((unsigned short)((stack[1].id>>10) & 0x3F),(unsigned short)(stack[0].id & 0x3ff));
                                }
                                if( mySys->isNetworkable() && !mySys->isNetworked())
                                {
                                        mySys->setNetworked();
                                }

                                // if this describes 'us', note any astro or VOC characteristics
                                // if this describes an alternate data frequency, note that.

                                if(mySys->isNetworked() && ((unsigned short)((stack[1].id>>10) & 0x3F) ==  mySys->siteId) ) {
                                        if(stack[0].grp) { // if active data channel
                                            if( stack[1].id & 8 ) {
                                                mySys->setSubType(" Astro");
                                            }
                                            if( stack[1].id & 32 ) {
                                                mySys->setOneChannel("/VOC");
                                            }
                                        } else {  // is an alternate data channel frequency
                                                mySys->noteAltDchan((unsigned short)(stack[0].id & 0x3ff));
                                        }
                                }
                        }*/
                }
                return;
        }

        /* astro call or 'coded pc grant' */

        if  (stack[2].cmd == OSW_FIRST_ASTRO || stack[2].cmd == OSW_FIRST_CODED_PC)
        {

                        if( stack[2].id )
                        {
													cout << " astro call or 'coded pc grant' " << stack[2].id << endl;

                                if(stack[2].cmd == OSW_FIRST_ASTRO) {
																				cout << " astro call or 'coded pc grant' " << stack[2].id << endl;
                                      //  mySys->setSubType(" Astro");
                                }
                                //twoOSWcall(OTHER_DIGITAL);
                        }

                numConsumed = 2;
                return;
        }

    // have handled all known dual-osw sequences above. these tend to be single or correlated

        switch( stack[2].cmd )
        {
        case OSW_SYS_NETSTAT:
								/*
                if( mySys && !mySys->isNetworkable())
                {
                        if( ++netCounts > 5 )
                        {
                                mySys->setNetworkable();
                                netCounts = 0;
                        }
                }*/


								// !!cout << "OSW_SYS_NETSTAT" << endl;


								//idlesWithNoId = 0;
                break;

        case OSW_SYS_STATUS: // system status


								//!!cout << "OSW_SYS_STATUS" << endl;


								/*
                if(mySys)
                {
                        register unsigned short statnum;

                        statnum = (unsigned short)((stack[2].id >> 13) & 7); // high order 3 bits opcode
                        if(statnum == 1) {
                                if(stack[2].id & ((unsigned short)0x1000)) {
                                        mySys->setBasicType("Type II");
                                } else {
                                        mySys->setBasicType("Type I");
                                }
                                statnum = (unsigned short)(stack[2].id >> 5);
                                statnum &= 7;
                                mySys->setConTone(tonenames[statnum]);
                        }
                }*/
                break;

        case OSW_SCAN_MARKER:
							cout << "OSW_SCAN_MARKER" << endl;
							/*
                noteIdentifier(stack[2].id,FALSE);
                if( osw_state == OperNewer )
                {
                        if( mySys)
                        {
                                mySys->setBasicType("Type II");
                        }
                }*/
                break;

        case OSW_TY1_EMERGENCY:         // type 1 emergency signal
								cout << "OSW_TY1_EMERGENCY" << endl;
                /*if(mySys)
                {
                        if(numStacked > 3 )
                        { // need to be unambiguous with type 2 call alert
                                blockNum = (unsigned short)((stack[2].id>>13)&7);
                                banktype = mySys->types[blockNum];
                                if( isTypeI(banktype) || isHybrid(banktype)) {
                                        mySys->noteEmergencySignal(typeIradio(stack[2].id,banktype));
                                } else {
                                        mySys->noteEmergencySignal(stack[2].id);
                                }
                        }
                }*/
                break;
/*
        case OSW_TY1_ALERT:
                if(mySys)
                {
                    blockNum = (unsigned short)((stack[2].id>>13)&7);
                    banktype = mySys->types[blockNum];
                    if( isTypeI(banktype) || isHybrid(banktype)) {
                        mySys->note_page(typeIradio(stack[2].id,banktype));
                    } else {
                        mySys->note_page(stack[2].id);
                    }
                }
                break;

        case OSW_CW_ID: // this seems to catch the appropriate diagnostic. thanks, wayne!
                if(mySys)
                {
                        if( (stack[2].id & 0xe000)  == 0xe000)
                        {
                                cwFreq = (unsigned short)(stack[2].id & 0x3ff);
                                mySys->noteCwId(cwFreq, stack[2].id & 0x1000);
                        } else {
                                static unsigned int lastDiag = 0;
                                static time_t lastT = 0;
                                if( (stack[2].id != lastDiag) || ((now - lastT) > 120))
                                {
                                        switch(stack[2].id & 0x0F00)
                                        {
                                case 0x0A00:
                                                sprintf(tempArea,"Diag %04x: %s Enabled",stack[2].id,getEquipName(stack[2].id&0x00ff));
                                                break;
                                case 0x0B00:
                                                sprintf(tempArea,"Diag %04x: %s Disabled",stack[2].id,getEquipName(stack[2].id&0x00ff));
                                                break;
                                case 0x0C00:
                                                sprintf(tempArea,"Diag %04x: %s Malfunction",stack[2].id,getEquipName(stack[2].id&0x00ff));
                                        break;
                                default:
                                                sprintf(tempArea,"Diagnostic code (other): %04x",stack[2].id);
                                        }
                                        mySys->logStringT(tempArea,DIAGNOST);
                                        lastT = now;
                                        lastDiag = stack[2].id;
                                }
                        }

                }
                break;*/

        default:
										//cout << "other default" <<endl;
                /*        if(stack[2].cmd >= OSW_TY1_STATUS_MIN &&
                           stack[2].cmd <= OSW_TY1_STATUS_MAX &&
                           numStacked > 4) {

                            // place to put type 1 status messages XXX

                            if(mySys && Verbose) {
                                mySys->note_text_msg(   stack[2].id,
                                                        "Status",
                                                        (unsigned short)(stack[2].cmd - OSW_TY1_STATUS_MIN)
                                                    );

                            }

                            break;
                        }

                        // this is the most frequent case that actually touches things....

                        if (stack[2].isFreq && (numStacked > 4)  ) {
                                if( mySys->vhf_uhf &&                         // we have such a system
                                    stack[1].isFreq &&      // this is a pair of frequencies
                                    (mySys->mapInpFreq(stack[2].cmd)==stack[1].cmd)  // the first frequency is input to second
                              ) {
                                        // special processing for vhf/uhf systems. They use the following
                                        // pairs during call setup:

                                            //    <originating radio id>           G/I <input frequency>
                                            //    <destination group or radio> G/I <output frequency>

                                    if( stack[1].grp ) {
                                        blockNum = (unsigned short)((stack[1].id>>13)&7);
                                        banktype = mySys->types[blockNum];
                                        // on uhf/vhf systems, only allowed to be type II!
                                        if( banktype != '0' && banktype != '2' ) {
                                                mySys->types[blockNum] = '2';
                                                mySys->note_type_2_block(blockNum);
                                        }
                                    }
                                    if( stack[1].grp && !stack[2].grp ) { // supposedly astro indication
                                        twoOSWcall(OTHER_DIGITAL);
                                    } else {
                                        twoOSWcall();
                                    }
                                    numConsumed = 2;
                                    return; // the return below pops only one word, we used 2 here

                                } else {
                                        // bare assignment
                                        // this is still a little iffy...one is not allowed to use IDs that
                                        // mimic the system id in an old system, so seems to work ok...
                                        if(((stack[2].id & 0xff00) == 0x1f00) && (osw_state == OperOlder)) {
                                                        noteIdentifier(stack[2].id,TRUE);
                                                        if( mySys)  {
                                                                mySys->noteDataChan(stack[2].cmd);
                                                        }
                                        } else {

                                        //              in the case of late entry message, just tickle current one
                                        //              if possible.... preserve flags since may be lost in single
                                        //              word case


                                        if( stack[2].grp) {
                                                blockNum = (unsigned short)((stack[2].id>>13)&7);
                                                banktype = mySys->types[blockNum];
                                if(isTypeII(banktype)) {
                                        mySys->type2Gcall(stack[2].cmd,                               // frequency
                                                          (unsigned short)(stack[2].id & 0xfff0),   // group ID
                                                          (unsigned short)(stack[2].id & 0x000f)    // flags
                                                         );
                                                        } else if(isTypeI(banktype)) {
                                                                mySys->typeIGcall(
                                                                                stack[2].cmd,                                           // frequency
                                                                                banktype,
                                                                                typeIgroup(stack[2].id,banktype),       // group ID
                                                                                typeIradio(stack[2].id,banktype)                                                     // calling ID
                                                                );
                                                        } else if(isHybrid(banktype)) {
                                                                mySys->hybridGcall(
                                                                                stack[2].cmd,                                           // frequency
                                                                                banktype,
                                                                                typeIgroup(stack[2].id,banktype),       // group ID
                                                                                typeIradio(stack[2].id,banktype)                                                     // calling ID
                                                                );
                                                        } else {
                                        mySys->type0Gcall(
                                                                                stack[2].cmd,                   // frequency
                                                                                stack[2].id,            // group ID
                                                                                0                                               // no flags
                                                                );
                                                    }

                                        } else {
                                                        mySys->type0Icall(
                                                                stack[2].cmd,           // freq#
                                                                stack[2].id,
                                                                FALSE,FALSE,FALSE);       // talker
                                                }
                                        }
                                }
                        }
                } else if( osw_state == GettingOldId ) {
                        if (stack[2].isFreq && ((stack[2].id & 0xff00) == 0x1f00)) {
                                noteIdentifier(stack[2].id,TRUE);
                                noteIdentifier(stack[2].id,TRUE);
                    }
                */
                break;
        }
        numConsumed = 1;
        return;
}



std::vector<TrunkMessage> SmartnetParser::parse_message(std::string s) {
	std::vector<TrunkMessage> messages;
	TrunkMessage message;

	char  tempArea [512];
        unsigned short blockNum;
        char banktype;
        unsigned short tt1,tt2;
        static unsigned int ott1,ott2;
				print_osw(s);
	message.message_type = UNKNOWN;
	message.encrypted = false;
	message.tdma = false;
	message.source = 0;
	message.sysid = 0;
	message.emergency = false;

	std::vector<std::string> x;
	boost::split(x, s, boost::is_any_of(","), boost::token_compress_on);

	int full_address = atoi( x[0].c_str() );
	int status = full_address & 0x000F;
	long address = full_address & 0xFFF0;
	int groupflag = atoi( x[1].c_str() );
	int command = atoi( x[2].c_str() );

	struct osw_stru bosw;
	bosw.id = address;
	bosw.grp = groupflag;
	bosw.cmd = command;
	struct osw_stru* Inposw = &bosw;
/*

	        // maintain a sliding stack of 5 OSWs. If previous iteration used more than one,
	        // don't utilize stack until all used ones have slid past.

	        switch(numStacked) // note: drop-thru is intentional!
	        {
	        case 5:
	        case 4:
	                stack[4] = stack[3];
	        case 3:
	                stack[3] = stack[2];
	        case 2:
	                stack[2] = stack[1];
	        case 1:
	                stack[1] = stack[0];
	        case 0:
	                stack[0] = *Inposw;
	                break;
	        default:
	                cout << "corrupt value for nstacked" << endl;
	                break;
	        }
	        if(numStacked < 5)
	        {
	                ++numStacked;
	        }
	        if( numConsumed > 0)
	        {
	                if(--numConsumed > 0)
	                {
	                        return messages;
	                }
	        }
	        if(  numStacked < 3 )
	        {
	                return messages; // at least need a window of 3 and 5 is better.
	        }

	x.clear();
	vector<string>().swap(x);


	cout << "0cmd: " << getfreq(stack[0].cmd) << " 0id: " << getfreq(stack[0].id) << " 1cmd: " << getfreq(stack[1].cmd) << " 1id: " << getfreq(stack[1].id) << " 0cmd: " << stack[0].cmd << " 0id: "<< stack[0].id << " 1cmd: " << stack[1].cmd << " 1id: "<< stack[1].id << endl;

	if(stack[2].cmd == OSW_FIRST_NORMAL || stack[2].cmd == OSW_FIRST_TY2AS1)
	{

					switch(stack[1].cmd)
					{
	case OSW_EXTENDED_FCN:

									if((stack[1].id & 0xfff0) == 0x26f0) { // ack type 2 text msg

											cout << "ack type 2 text msg " <<  stack[2].id << " Msg: " << (stack[1].id & (unsigned short)15) << endl;

									} else if((stack[1].id & 0xfff8) == 0x26E0) { // ack type 2 status

													cout << "ack type 2 status " <<  stack[2].id << " Status " << (stack[1].id & (unsigned short)7) << endl;

									} else if( (stack[0].id & 0xff00) == 0x1F00 &&
										 (stack[1].id & 0xfc00) == 0x2800 && stack[0].cmd == (stack[1].id & 0x3FF))
									{
													// non-smartzone identity:  sysid[10]
													//	 <sysid>              x   308
												//		 <001010><ffffffffff> x   30b
													//	 1Fxx                 x   <ffffffffff>

													cout << "non-smartzone identity, data chan: " << stack[0].cmd << endl;
													message.sysid = stack[0].cmd;
													message.message_type = SYSID;
													numConsumed = 3; // we used up all 3
													messages.push_back(message);
													return messages;
									} else if((stack[1].id & 0xfc00) == 0x2800)
									{
													// smartzone identity: sysid[10]

													//			<sysid>              x   308
													//			<001010><ffffffffff> x   30b

													cout << "smartzone identity:" << stack[2].id << " data chan: " << (unsigned short)(stack[1].id & 0x03ff) << endl;
													message.sysid = stack[2].cmd;
													message.message_type = SYSID;
									} else if((stack[1].id & 0xfc00) == 0x6000)
									{
													// smartzone peer info:                         sysid[24]
													//								<sysid>              x   308
													//								<011000><ffffffffff> x   30b

													// evidence of smartzone, but can't use this pair for id
													//idlesWithNoId = 0; // got identifying info...
													cout << " smartzone peer info: " << endl;

									} else if((stack[1].id & 0xfc00) == 0x2400)
									{ // various
													//idlesWithNoId = 0;

													if( (stack[1].id & 0x03ff) == 0x21c)
													{
														cout << " affiliation: " << stack[2].id << endl;

													 }
									} else if( stack[1].id  == 0x2021) {
											cout << " mscancel: " << stack[2].id;


									} else {
													//idlesWithNoId = 0;

	 // unknown sequence.
	 //example: <000000> fffa x 308 then 0321 x 30b gateway close?
	 //example: <001000> 3890 G 308      2021 G 30b
	//					<001011> 811d I 308      2c4a I 30b
	//					<001001> 83d3 I 308      261c I 30b

	// known:   <011000>
	//					<001010>



									}
									numConsumed = 2;
									messages.push_back(message);
									return messages;

	case 0x320:
									if(stack[0].cmd == OSW_EXTENDED_FCN)
									{
													// definitely smart-zone: 308 320 30b
													//  <sysId>                             308
													//  <cell id 6 bits><10 bits see below> 320
													//      <011000><ffffffffff>            30b
													//

													//		10 bits:  bbb0v?AaES

													//				bbb - band: [800,?,800,821,900,?,?]
													//		32  v   - VOC
													//		8   A   - Astro
													//		4   a   - analog
													//		2   E   - 12kbit encryption
													//		1   S   - site is Active (if zero?)

													numConsumed = 3;
													//idlesWithNoId = 0; // got identifying info...
													if( (stack[0].id & 0xfc00) == 0x6000)
													{

															cout << "something about a peer" << endl;
															message.talkgroup = stack[0].id;
															message.freq = getfreq(stack[0].cmd);
															// Channel Grant
														 message.message_type = GRANT;
														 message.source = stack[1].id;
														 // Check Status

															cout << "Status: " << status << endl;
														 if(status == 2 && status == 4 && status == 5) {
															 message.emergency = true;
														 } else if ( status >= 8 ) { // Ignore DES Encryption
															 message.message_type = UNKNOWN;
														 }*/
																	/*if(mySys && (stack[2].id == mySys->sysId) )
																	{
																					mySys->note_neighbor((unsigned short)((stack[1].id>>10) & 0x3F),(unsigned short)(stack[0].id & 0x3ff));
																					if( mySys->isNetworkable() && !mySys->isNetworked())
																					{
																									mySys->setNetworked();
																					}

																					// if this describes 'us', note any astro or VOC characteristics
																					// for alternate data channel, note that.

																					if(mySys->isNetworked() && ((unsigned short)((stack[1].id>>10) & 0x3F) ==  mySys->siteId) ) {
																									if(stack[0].grp) { // if active data channel
																													if( stack[1].id & 8 ) {
																																	mySys->setSubType(" Astro");
																													}
																													if( stack[1].id & 32 ) {
																																	mySys->setOneChannel("/VOC");
																													}
																									} else {  // is an alternate data channel frequency
																													mySys->noteAltDchan((unsigned short)(stack[0].id & 0x3ff));
																									}
																					}
																	}*/
												/*	}
													messages.push_back(message);
													return messages;
									}
									break;

	case OSW_TY2_AFFILIATION:
									// type II requires double word 308/310. single word version is type 1 status,
									// whatever that means.
									cout << "OSW_TY2_AFFILIATION: " << stack[2].id <<  " and " << stack[1].id << endl;

									break;

	case OSW_TY2_MESSAGE:
	//        sprintf(tempArea,"Radio %04x, Message %d [1..8]\n",stack[2].id,1+(stack[1].id&7));

									cout << "OSW_TY2_MESSAGE: " << stack[2].id << " MSG: " <<   (unsigned short)(stack[1].id&7) << endl;

									break;

	case OSW_TY2_CALL_ALERT:
									// type II call alert. we can ignore the 'ack', 0x31A
									cout << "OSW_TY2_CALL_ALERT" << endl;

									break;

	case OSW_SYSTEM_CLOCK:  // system clock

													tt1 = stack[2].id;
													tt2 = stack[1].id;
													if( tt2 != ott2 || tt1 != ott1 )
													{
																	ott2 = tt2;
																	ott1 = tt1;
																	sprintf(tempArea,"%02u/%02u/%02u %02u:%02u",
																					(tt1>>5)& 0x000f,
																					(tt1)   & 0x001f,
																					(tt1>>9),
																					(tt2>>8)& 0x001f,
																					(tt2)   & 0x00ff);
																	cout << "OSW_SYSTEM_CLOCK: " << tempArea << endl;
													}

									break;

	case OSW_EMERG_ANNC:
									cout << "OSW_EMERG_ANNC: " << stack[2].id << endl;

									break;
	case OSW_AFFIL_FCN:
									cout << "OSW_AFFIL_FCN - ";

													switch(stack[1].id & 0x000f)
													{
													case 0:
													case 1:
													case 2:
													case 3:
													case 4:
													case 5:
													case 6:
													case 7:
																	cout << "Status: " << stack[2].id << " - " << (unsigned short)(stack[1].id & 0x0007) << endl;

																	break;
													case 8:
																	cout << "Emergency Signal: " << stack[2].id << endl;
																	//mySys->noteEmergencySignal(stack[2].id);
																	break;
													case 9: // ChangeMe request
													case 10:// Ack Dynamic Group ID
													case 11:// Ack Dynamic Multi ID
																	break;
													default:
																	break;
													}

									break;

	// Type I Dynamic Regrouping ("patch")

	case 0x340: // size code A (motorola codes)
	case 0x341: // size code B
	case 0x342: // size code C
	case 0x343: // size code D
	case 0x344: // size code E
	case 0x345: // size code F
	case 0x346: // size code G
	case 0x347: // size code H
	case 0x348: // size code I
	case 0x349: // size code J
	case 0x34a: // size code K

													// patch notification
													// patch 'tag' is in stack[1].id
													//group associated is stack[2].id

													switch(stack[2].id & 7)
													{
															case 0:  cout << " normal for type 1 groups ";
															case 3:  cout <<  "patch ";
															case 4:  cout << "emergency patch ";
																	cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " more: " << (char)('A'+(stack[1].cmd - 0x340)) << endl;

																	break;
															case 5:
															case 7:
																	cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " more: " << (char)('A'+(stack[1].cmd - 0x340)) << endl;
																	break;
															default:
																	break;
													}

									break;
	case 0x34c: // size code M trunker = L

													// patch notification
													// patch 'tag' is in stack[1].id
													//group associated is stack[2].id
													switch(stack[2].id & 7)
													{
															case 0: cout << " normal for type 1 groups ";
															case 3: cout <<  "patch ";
															case 4: cout << "emergency patch ";
																	cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " L" << endl;
																	break;
															case 5:
															case 7:
																	cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " L" << endl;
																	break;
															default:
																	break;
													}

									break;
	case 0x34e: // size code O Trunker = M

													// patch notification
													// patch 'tag' is in stack[1].id
													//group associated is stack[2].id
													switch(stack[2].id & 7)
													{
															case 0: cout << " normal for type 1 groups ";
															case 3: cout <<  "patch ";
															case 4: cout << "emergency patch ";


																		cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " M" << endl;
																		break;
															case 5:
															case 7:
																	cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " M" << endl;

																	break;
															default:
																	break;
													}

									break;

	case 0x350: // size code Q Trunker = N

													// patch notification
													// patch 'tag' is in stack[1].id
													//group associated is stack[2].id
													switch(stack[2].id & 7)
													{
															case 0: cout << " normal for type 1 groups ";
															case 3: cout <<  "patch ";
															case 4: cout << "emergency patch ";


																	cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " N" << endl;

																	break;
															case 5:
															case 7:
															cout << " tag: " << stack[1].id << " Group: " << stack[2].id << " N" << endl;

																	break;
															default:
																	break;
													}

									break;
	default:
					cout << "default" << endl;
					*/
/*
													if(stack[1].isFreq)
													{
																	if ( stack[2].cmd == OSW_FIRST_TY2AS1 && stack[1].grp)
																	{
																					blockNum = (unsigned short)((stack[1].id>>13)&7);
																					banktype = mySys->types[blockNum];
																					if( banktype == '2' )
																					{
																									mySys->types[blockNum] = '?';
																									_settextposition(FBROW,1);
																									sprintf(tempArea,"Block %d <0..7> is Type IIi                         ",blockNum);
																									_outtext(tempArea);
																					}
																					mySys->setSubType("i");
																	}
																	if( stack[2].id != 0 )
																	{ // zero id used special way in hybrid system
																					twoOSWcall();
																	}
													}*/

									// otherwise 'busy', others...
	/*				}

					numConsumed = 2;
					messages.push_back(message);
					return messages;
	}

	// uhf/vhf equivalent of 308/320/30b

	if( stack[1].cmd == 0x0320 && stack[0].cmd == OSW_EXTENDED_FCN )

	{
					numConsumed = 3;
					//idlesWithNoId = 0; // got identifying info...
					if( (stack[0].id & 0xfc00) == 0x6000)
					{
							cout << "uhf/vhf equivalent of 308/320/30b" << endl;
							cout << "0cmd: " << getfreq(stack[0].cmd) << " 0id: " << getfreq(stack[0].id) << " 1cmd: " << getfreq(stack[1].cmd) << " 1id: " << getfreq(stack[1].id);
							message.talkgroup = stack[0].id;
							message.freq = getfreq(stack[0].cmd);
							// Channel Grant
						 message.message_type = GRANT;
						 message.source = stack[1].id;
						 // Check Status

							cout << "Status: " << status << endl;
						 if(status == 2 && status == 4 && status == 5) {
							 message.emergency = true;
						 } else if ( status >= 8 ) { // Ignore DES Encryption
							 message.message_type = UNKNOWN;
						 }*/
							/*
									if(mySys && (stack[2].id == mySys->sysId) )
									{
													if( myFreqMap->isFrequency((unsigned short)(stack[0].id&0x3ff),mySys->getBandPlan()) )
													{
																	mySys->note_neighbor((unsigned short)((stack[1].id>>10) & 0x3F),(unsigned short)(stack[0].id & 0x3ff));
													}
													if( mySys->isNetworkable() && !mySys->isNetworked())
													{
																	mySys->setNetworked();
													}

													// if this describes 'us', note any astro or VOC characteristics
													// if this describes an alternate data frequency, note that.

													if(mySys->isNetworked() && ((unsigned short)((stack[1].id>>10) & 0x3F) ==  mySys->siteId) ) {
																	if(stack[0].grp) { // if active data channel
																			if( stack[1].id & 8 ) {
																					mySys->setSubType(" Astro");
																			}
																			if( stack[1].id & 32 ) {
																					mySys->setOneChannel("/VOC");
																			}
																	} else {  // is an alternate data channel frequency
																					mySys->noteAltDchan((unsigned short)(stack[0].id & 0x3ff));
																	}
													}
									}*/
	/*				}
					messages.push_back(message);
					return messages;
	}

	// astro call or 'coded pc grant'

	if  (stack[2].cmd == OSW_FIRST_ASTRO || stack[2].cmd == OSW_FIRST_CODED_PC)
	{

									if( stack[2].id )
									{
										cout << " astro call or 'coded pc grant' " << stack[2].id << endl;

													if(stack[2].cmd == OSW_FIRST_ASTRO) {
																	cout << " astro call or 'coded pc grant' " << stack[2].id << endl;
																//  mySys->setSubType(" Astro");
													}
													//twoOSWcall(OTHER_DIGITAL);
									}

					numConsumed = 2;
					messages.push_back(message);
					return messages;
	}

// have handled all known dual-osw sequences above. these tend to be single or correlated

	switch( stack[2].cmd )
	{
	case OSW_SYS_NETSTAT:



					// !!cout << "OSW_SYS_NETSTAT" << endl;


					//idlesWithNoId = 0;
					break;

	case OSW_SYS_STATUS: // system status


					//!!cout << "OSW_SYS_STATUS" << endl;

*/
					/*
					if(mySys)
					{
									register unsigned short statnum;

									statnum = (unsigned short)((stack[2].id >> 13) & 7); // high order 3 bits opcode
									if(statnum == 1) {
													if(stack[2].id & ((unsigned short)0x1000)) {
																	mySys->setBasicType("Type II");
													} else {
																	mySys->setBasicType("Type I");
													}
													statnum = (unsigned short)(stack[2].id >> 5);
													statnum &= 7;
													mySys->setConTone(tonenames[statnum]);
									}
					}*/
	/*				break;

	case OSW_SCAN_MARKER:
				cout << "OSW_SCAN_MARKER" << endl;

					break;

	case OSW_TY1_EMERGENCY:         // type 1 emergency signal
					cout << "OSW_TY1_EMERGENCY" << endl;

					break;*/
/*
	case OSW_TY1_ALERT:
					if(mySys)
					{
							blockNum = (unsigned short)((stack[2].id>>13)&7);
							banktype = mySys->types[blockNum];
							if( isTypeI(banktype) || isHybrid(banktype)) {
									mySys->note_page(typeIradio(stack[2].id,banktype));
							} else {
									mySys->note_page(stack[2].id);
							}
					}
					break;

	case OSW_CW_ID: // this seems to catch the appropriate diagnostic. thanks, wayne!
					if(mySys)
					{
									if( (stack[2].id & 0xe000)  == 0xe000)
									{
													cwFreq = (unsigned short)(stack[2].id & 0x3ff);
													mySys->noteCwId(cwFreq, stack[2].id & 0x1000);
									} else {
													static unsigned int lastDiag = 0;
													static time_t lastT = 0;
													if( (stack[2].id != lastDiag) || ((now - lastT) > 120))
													{
																	switch(stack[2].id & 0x0F00)
																	{
													case 0x0A00:
																					sprintf(tempArea,"Diag %04x: %s Enabled",stack[2].id,getEquipName(stack[2].id&0x00ff));
																					break;
													case 0x0B00:
																					sprintf(tempArea,"Diag %04x: %s Disabled",stack[2].id,getEquipName(stack[2].id&0x00ff));
																					break;
													case 0x0C00:
																					sprintf(tempArea,"Diag %04x: %s Malfunction",stack[2].id,getEquipName(stack[2].id&0x00ff));
																	break;
													default:
																					sprintf(tempArea,"Diagnostic code (other): %04x",stack[2].id);
																	}
																	mySys->logStringT(tempArea,DIAGNOST);
																	lastT = now;
																	lastDiag = stack[2].id;
													}
									}

					}
					break;*/

//	default:
							//cout << "other default" <<endl;
					/*        if(stack[2].cmd >= OSW_TY1_STATUS_MIN &&
										 stack[2].cmd <= OSW_TY1_STATUS_MAX &&
										 numStacked > 4) {

											// place to put type 1 status messages XXX

											if(mySys && Verbose) {
													mySys->note_text_msg(   stack[2].id,
																									"Status",
																									(unsigned short)(stack[2].cmd - OSW_TY1_STATUS_MIN)
																							);

											}

											break;
									}

									// this is the most frequent case that actually touches things....

									if (stack[2].isFreq && (numStacked > 4)  ) {
													if( mySys->vhf_uhf &&                         // we have such a system
															stack[1].isFreq &&      // this is a pair of frequencies
															(mySys->mapInpFreq(stack[2].cmd)==stack[1].cmd)  // the first frequency is input to second
												) {
																	// special processing for vhf/uhf systems. They use the following
																	// pairs during call setup:

																			//    <originating radio id>           G/I <input frequency>
																			//    <destination group or radio> G/I <output frequency>

															if( stack[1].grp ) {
																	blockNum = (unsigned short)((stack[1].id>>13)&7);
																	banktype = mySys->types[blockNum];
																	// on uhf/vhf systems, only allowed to be type II!
																	if( banktype != '0' && banktype != '2' ) {
																					mySys->types[blockNum] = '2';
																					mySys->note_type_2_block(blockNum);
																	}
															}
															if( stack[1].grp && !stack[2].grp ) { // supposedly astro indication
																	twoOSWcall(OTHER_DIGITAL);
															} else {
																	twoOSWcall();
															}
															numConsumed = 2;
															return; // the return below pops only one word, we used 2 here

													} else {
																	// bare assignment
																	// this is still a little iffy...one is not allowed to use IDs that
																	// mimic the system id in an old system, so seems to work ok...
																	if(((stack[2].id & 0xff00) == 0x1f00) && (osw_state == OperOlder)) {
																									noteIdentifier(stack[2].id,TRUE);
																									if( mySys)  {
																													mySys->noteDataChan(stack[2].cmd);
																									}
																	} else {

																	//              in the case of late entry message, just tickle current one
																	//              if possible.... preserve flags since may be lost in single
																	//              word case


																	if( stack[2].grp) {
																					blockNum = (unsigned short)((stack[2].id>>13)&7);
																					banktype = mySys->types[blockNum];
													if(isTypeII(banktype)) {
																	mySys->type2Gcall(stack[2].cmd,                               // frequency
																										(unsigned short)(stack[2].id & 0xfff0),   // group ID
																										(unsigned short)(stack[2].id & 0x000f)    // flags
																									 );
																									} else if(isTypeI(banktype)) {
																													mySys->typeIGcall(
																																					stack[2].cmd,                                           // frequency
																																					banktype,
																																					typeIgroup(stack[2].id,banktype),       // group ID
																																					typeIradio(stack[2].id,banktype)                                                     // calling ID
																													);
																									} else if(isHybrid(banktype)) {
																													mySys->hybridGcall(
																																					stack[2].cmd,                                           // frequency
																																					banktype,
																																					typeIgroup(stack[2].id,banktype),       // group ID
																																					typeIradio(stack[2].id,banktype)                                                     // calling ID
																													);
																									} else {
																	mySys->type0Gcall(
																																					stack[2].cmd,                   // frequency
																																					stack[2].id,            // group ID
																																					0                                               // no flags
																													);
																							}

																	} else {
																									mySys->type0Icall(
																													stack[2].cmd,           // freq#
																													stack[2].id,
																													FALSE,FALSE,FALSE);       // talker
																					}
																	}
													}
									}
					} else if( osw_state == GettingOldId ) {
									if (stack[2].isFreq && ((stack[2].id & 0xff00) == 0x1f00)) {
													noteIdentifier(stack[2].id,TRUE);
													noteIdentifier(stack[2].id,TRUE);
							}
					*/
	/*				break;
	}
	numConsumed = 1;
	messages.push_back(message);
	return messages;

*/

	if (((command >= 0x340) && (command <= 0x34E)) || (command == 0x350)) {

		cout << "Patching Command: " << hex << command << " Last CMD: 0x" <<  hex << lastcmd << " Freq: " << message.freq << " Talkgroup: " << dec << address << " Last Address: " << dec << lastaddress<< endl;

	}
	if ((address & 0xfc00) == 0x2800) {
		message.sysid = lastaddress;
		message.message_type = SYSID;
	} else if (command < 0x2d0) {
		message.talkgroup = address;
		message.freq = getfreq(command);
		//cout << "Command: " << hex << command << " Last CMD: 0x" <<  hex << lastcmd << " Freq: " << message.freq << " Talkgroup: " << dec << address << " Last Address: " << dec << lastaddress<< endl;

		if ( lastcmd == 0x308 || lastcmd == 0x321  || lastcmd == 0x30b) { // Include digital

			/* Status Message in TalkGroup ID
			 *   0 Normal Talkgroup
			 *   1 All Talkgroup
			 *   2 Emergency
			 *   3 Talkgroup patch to another
			 *   4 Emergency Patch
			 *   5 Emergency multi - group
			 *   6 Not assigned
			 *   7 Multi - select (initiated by dispatcher)
			 *   8 DES Encryption talkgroup
			 *   9 DES All Talkgroup
			 *  10 DES Emergency
			 *  11 DES Talkgroup patch
			 *  12 DES Emergency Patch
			 *  13 DES Emergency multi - group
			 *  14 Not assigned
			 *  15 Multi - select DES TG
			 */

			 // Channel Grant
			message.message_type = GRANT;
			message.source = lastaddress;
			// Check Status
			cout << "Grant Command: " << hex << command << " Last CMD: 0x" <<  hex << lastcmd << " Freq: " << message.freq << " Talkgroup: " << dec << address << " Source: " << dec << lastaddress<< endl;

			 cout << "Status: " << status << endl;
			if(status == 2 || status == 4 || status == 5) {
				message.emergency = true;
			} else if ( status >= 8 ) { // Ignore DES Encryption
				message.message_type = UNKNOWN;
			}
		} else if ( lastcmd == 0x3bf) {
			// Call continuation
			message.message_type = UPDATE;
		}
	} else if (command == 0x03c0) {
		message.message_type = STATUS;
		//cout << "Status Command: " << hex << command << " Last CMD: 0x" <<  hex << lastcmd << " Freq: " << message.freq << " Talkgroup: " << dec << address << " Last Address: " << dec << lastaddress<< endl;

		//parse_status(command, address,groupflag);
	}

	lastaddress = full_address;
	lastcmd = command;
	//messages.push_back(message);
	return messages;
}

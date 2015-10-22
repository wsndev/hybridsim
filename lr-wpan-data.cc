/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 The Boeing Company
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Tom Henderson <thomas.r.henderson@boeing.com>
 */

/*
 * Try to send data end-to-end through a LrWpanMac <-> LrWpanPhy <->
 * SpectrumChannel <-> LrWpanPhy <-> LrWpanMac chain
 *
 * Trace Phy state changes, and Mac DataIndication and DataConfirm events
 * to stdout
 */
#include <ns3/log.h>
#include <ns3/core-module.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/packet.h>

#include <iostream>
#include <iomanip>
#include <iosfwd>
#include <string>

using namespace ns3;

const char* output = NULL;
const char* input = NULL;

static void DataIndication (McpsDataIndicationParams params, Ptr<Packet> p) {
	NS_LOG_UNCOND ("Received packet of size " << p->GetSize ());

	LrWpanMacTrailer trailer;
	p->PeekTrailer(trailer);

	uint8_t temp[2];
	uint32_t size = p->GetSize();
	uint8_t mac_payload[127];
	p->CopyData(mac_payload, size);
	uint8_t frame_length = size + 11;

	uint8_t frame_type = 1;
	bool sec_enabled = 0;
	bool frame_pending = 0;
	bool ack_request = 0;
	bool intra_pan = 0;
	uint8_t dst_addr_mode = params.m_dstAddrMode;
	uint8_t frame_version = 1;
	uint8_t src_addr_mode = params.m_dstAddrMode;
	uint8_t sequence_num = params.m_dsn;
	uint16_t dst_pan_id = params.m_dstPanId;
	params.m_dstAddr.CopyTo(temp);
	uint16_t dst_addr = (uint16_t) temp[0] << 8 | (uint16_t) temp[1];
	uint16_t src_pan_id = params.m_srcPanId;
	params.m_srcAddr.CopyTo(temp);
	uint16_t src_addr = (uint16_t) temp[0] << 8 | (uint16_t) temp[1];
	uint16_t crc16 = trailer.GetFcs();

	FILE * fp;
	fp = fopen(output, "w");
	if (fp == NULL){
		exit(EXIT_FAILURE);
	}

	fprintf(fp, "%d: frame type\n", frame_type);
	fprintf(fp, "%d: sec enabled\n", sec_enabled);
    fprintf(fp, "%d: frame pending\n", frame_pending);
    fprintf(fp, "%d: ACK request\n", ack_request);
    fprintf(fp, "%d: intra PAN\n", intra_pan);
    fprintf(fp, "%d: dst addr mode\n", dst_addr_mode);
    fprintf(fp, "%d: frame version\n", frame_version);
    fprintf(fp, "%d: src addr mode\n", src_addr_mode);
	fprintf(fp, "%d: sequence num\n", sequence_num);
    fprintf(fp, "%04x: dst PAN id\n", dst_pan_id);
	fprintf(fp, "%04x: dst addr\n", dst_addr);
	fprintf(fp, "%04x: src PAN id\n", src_pan_id);
    fprintf(fp, "%04x: src add\n", src_addr);
	uint8_t i;
	for (i = 0; i < size; ++i){
		fprintf(fp, "%02x ", mac_payload[i]);
	}
	fprintf(fp, "\n");
	fprintf(fp, "%04x: crc16-ccitt\n", crc16);
	fprintf(fp, "%d: frame length\n", frame_length);

	for (i = 0; i < size; ++i){
		if (mac_payload[i] >= 0x20 && mac_payload[i] < 0x7f){
			fprintf(fp, "%c", mac_payload[i]);
		}
		else{
			fprintf(fp, ".");
		}
	}
	fprintf(fp, "\n");

	fclose(fp);
}

static void DataConfirm (McpsDataConfirmParams params) {
	NS_LOG_UNCOND ("LrWpanMcpsDataConfirmStatus = " << params.m_status);
}

static void StateChangeNotification (std::string context, Time now, LrWpanPhyEnumeration oldState, LrWpanPhyEnumeration newState) {
	NS_LOG_UNCOND (context << " state change at " << now.GetSeconds ()
				   << " from " << LrWpanHelper::LrWpanPhyEnumerationPrinter (oldState)
				   << " to " << LrWpanHelper::LrWpanPhyEnumerationPrinter (newState));
}

int main (int argc, char *argv[]) {

	// char* inputFile;
	std::string inputFile;
	std::string outputFile;
	// int hop = -1;

	bool verbose = false;

	CommandLine cmd;

	cmd.AddValue ("verbose", "turn on all log components", verbose);
	cmd.AddValue ("inputFile", "input file for packets", inputFile);
	cmd.AddValue ("outputFile", "input file for packets", outputFile);
	// cmd.AddValue ("hop", "next hop", hop);

	cmd.Parse (argc, argv);

	if (inputFile.empty() || outputFile.empty()){
		NS_LOG_UNCOND ("inputFile send");
		exit(EXIT_FAILURE);	
	}

	output = outputFile.c_str();
	input = inputFile.c_str();

	FILE * fp;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	fp = fopen(input, "r");
	if (fp == NULL){
		exit(EXIT_FAILURE);	
	}

	uint8_t frame_type = 0;
	bool sec_enabled = 0;
	bool frame_pending = 0;
	bool ack_request = 0;
	bool intra_pan = 0;
	uint8_t dst_addr_mode = 0;
	uint8_t frame_version = 0;
	uint8_t src_addr_mode = 0;
	uint8_t sequence_num = 0;
	uint16_t dst_pan_id = 0;
	uint16_t dst_addr = 0;
	uint16_t src_pan_id = 0;
	uint16_t src_addr = 0;
	uint8_t mac_payload[127];
	uint8_t payload_length = 0;

    size_t i;
	uint8_t line_num = 0;
	char temp[2];
	while ((read = getline(&line, &len, fp)) != -1) {
		for (i = 0; i < len && line_num < 13; ++i){
			if (line[i] == ':'){
				line[i] = '\0';
			}
		}
		switch (line_num){
			case 0:// frame type
				frame_type = atoi(line);
				NS_LOG_UNCOND((uint16_t) frame_type << ": frame type");
				break;
            case 1:// sec enabled
				sec_enabled = atoi(line);
				NS_LOG_UNCOND((uint16_t) sec_enabled << ": sec enabled");
                break;
            case 2:// frame pending
				frame_pending = atoi(line);
                NS_LOG_UNCOND((uint16_t) frame_pending << ": frame pending");
                break;
            case 3:// ACK request
				ack_request = atoi(line);
                NS_LOG_UNCOND((uint16_t) ack_request << ": ACK request");
                break;
            case 4:// intra PAN
				intra_pan = atoi(line);
                NS_LOG_UNCOND((uint16_t) intra_pan << ": intra PAN");
                break;
            case 5:// dst addr mode
				dst_addr_mode = atoi(line);
		        NS_LOG_UNCOND((uint16_t) dst_addr_mode << ": dst addr mode");
		        break;
            case 6:// frame version
				frame_version = atoi(line);
                NS_LOG_UNCOND((uint16_t) frame_version << ": frame version");
                break;
			case 7:// src addr mode
				src_addr_mode = atoi(line);
                NS_LOG_UNCOND((uint16_t) src_addr_mode << ": src addr mode");
				break;
			case 8:// sequence num
				sequence_num = atoi(line);
				NS_LOG_UNCOND((uint16_t) sequence_num << ": sequence num");
				break;
            case 9:// dst PAN id
				dst_pan_id = strtoul(line, NULL, 16);
		        NS_LOG_UNCOND(std::setfill('0') << std::setw(4) << std::hex << dst_pan_id << ": dst PAN id");
		        break;
            case 10:// dst addr
				dst_addr = strtoull(line, NULL, 16);
				NS_LOG_UNCOND(std::setfill('0') << std::setw(4) << std::hex << dst_addr << ": dst addr");
                break;
            case 11:// src PAN id
				src_pan_id = strtoul(line, NULL, 16);
				NS_LOG_UNCOND(std::setfill('0') << std::setw(4) << std::hex << src_pan_id << ": src PAN id");
                break;
            case 12:// src addr
				src_addr = strtoull(line, NULL, 16);
		        NS_LOG_UNCOND(std::setfill('0') << std::setw(4) << std::hex << src_addr << ": src addr");
                break;
            case 13:// mac data unit
            	std::ostringstream strstream;
				payload_length = strlen(line) / 3;
				for (i = 0; i < payload_length; ++i){
					temp[0] = line[i * 3 + 0];
					temp[1] = line[i * 3 + 1];
					mac_payload[i] = strtoul(temp, NULL, 16);
					strstream << std::setfill('0') << std::setw(2) << std::hex << (uint16_t) mac_payload[i] << " ";
				}
				NS_LOG_UNCOND(strstream.str());
				break;
		}
		++line_num;
	}
	// printf("%s\n", mac_payload);

	fclose(fp);
	if (line){
		free(line);
	}

	LrWpanHelper lrWpanHelper;
	if (verbose) {
		lrWpanHelper.EnableLogComponents ();
	}

	// Enable calculation of FCS in the trailers. Only necessary when interacting with real devices or wireshark.
	// GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

	// Create 2 nodes, and a NetDevice for each one
	Ptr<Node> n0 = CreateObject <Node> ();
	Ptr<Node> n1 = CreateObject <Node> ();

	Ptr<LrWpanNetDevice> dev0 = CreateObject<LrWpanNetDevice> ();
	Ptr<LrWpanNetDevice> dev1 = CreateObject<LrWpanNetDevice> ();

	const uint8_t b_src_addr[2] = {(uint8_t) (src_addr >> 8), (uint8_t) src_addr};
	Mac16Address srcAddr;
	srcAddr.CopyFrom(b_src_addr);

	const uint8_t b_dst_addr[2] = {(uint8_t) (dst_addr >> 8), (uint8_t) dst_addr};
	Mac16Address dstAddr;
	dstAddr.CopyFrom(b_dst_addr);

	dev0->SetAddress (srcAddr);
	dev1->SetAddress (dstAddr);
	dev0->GetMac()->m_macPanId = src_pan_id;
	dev1->GetMac()->m_macPanId = dst_pan_id;

	// Each device must be attached to the same channel
	Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
	Ptr<LogDistancePropagationLossModel> propModel = CreateObject<LogDistancePropagationLossModel> ();
	Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
	channel->AddPropagationLossModel (propModel);
	channel->SetPropagationDelayModel (delayModel);

	dev0->SetChannel (channel);
	dev1->SetChannel (channel);

	// To complete configuration, a LrWpanNetDevice must be added to a node
	n0->AddDevice (dev0);
	n1->AddDevice (dev1);

	// Trace state changes in the phy
	dev0->GetPhy ()->TraceConnect ("TrxState", std::string ("phy0"), MakeCallback (&StateChangeNotification));
	dev1->GetPhy ()->TraceConnect ("TrxState", std::string ("phy1"), MakeCallback (&StateChangeNotification));

	Ptr<ConstantPositionMobilityModel> sender0Mobility = CreateObject<ConstantPositionMobilityModel> ();
	sender0Mobility->SetPosition (Vector (0,0,0));
	dev0->GetPhy ()->SetMobility (sender0Mobility);
	Ptr<ConstantPositionMobilityModel> sender1Mobility = CreateObject<ConstantPositionMobilityModel> ();
	// Configure position 10 m distance
	sender1Mobility->SetPosition (Vector (0,10,0));
	dev1->GetPhy ()->SetMobility (sender1Mobility);

	McpsDataConfirmCallback cb0;
	cb0 = MakeCallback (&DataConfirm);
	dev0->GetMac ()->SetMcpsDataConfirmCallback (cb0);

	McpsDataIndicationCallback cb1;
	cb1 = MakeCallback (&DataIndication);
	dev0->GetMac ()->SetMcpsDataIndicationCallback (cb1);

	McpsDataConfirmCallback cb2;
	cb2 = MakeCallback (&DataConfirm);
	dev1->GetMac ()->SetMcpsDataConfirmCallback (cb2);

	McpsDataIndicationCallback cb3;
	cb3 = MakeCallback (&DataIndication);
	dev1->GetMac ()->SetMcpsDataIndicationCallback (cb3);

	// Tracing
	lrWpanHelper.EnablePcapAll (std::string ("lr-wpan-data"), true);
	AsciiTraceHelper ascii;
	Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("lr-wpan-data.tr");
	lrWpanHelper.EnableAsciiAll (stream);


	// The below should trigger two callbacks when end-to-end data is working
	// 1) DataConfirm callback is called
	// 2) DataIndication callback is called with value of 50
	Ptr<Packet> p0 = Create<Packet> (mac_payload, payload_length);  // 50 bytes of dummy data
	McpsDataRequestParams params;
	params.m_srcAddrMode = (LrWpanAddressMode) src_addr_mode;
	params.m_dstAddrMode = (LrWpanAddressMode) dst_addr_mode;
	params.m_dstPanId = dst_pan_id;
	params.m_dstAddr = dstAddr;
	params.m_msduHandle = 0;
	// params.m_txOptions = TX_OPTION_ACK;
//  dev0->GetMac ()->McpsDataRequest (params, p0);
	Simulator::ScheduleWithContext (1, Seconds (0.0),
									&LrWpanMac::McpsDataRequest,
									dev0->GetMac (), params, p0);
	Simulator::Run ();

	Simulator::Destroy ();
	return 0;
}

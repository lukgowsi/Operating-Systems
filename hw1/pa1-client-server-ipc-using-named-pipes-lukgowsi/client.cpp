#include <chrono>
/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Lucas Ho
	UIN: 930009783
	Date: 9/13/22
*/
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	int m = MAX_MESSAGE;
	string filename = "";
	bool newChannel = false;
	vector<FIFORequestChannel*> channels;

	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				m = atoi (optarg);
				break;
			case 'c':
				newChannel = true;
				break;
		}
	}

	int mcop = m;

	string str = to_string(mcop);
	vector<char> chars(str.begin(), str.end());
	chars.push_back('\0');

	char *pchar = &chars[0]; 

	//give arguments for the server
	//server needs './server', '-m', '<val for -m arg>', 'NULL'
	int pid = fork(); //fork

	//in the child, run execvp using the server arguments
	char* argument_list[] = {(char*)"./server", (char*)"-m", pchar, NULL};

	if (pid == 0){
		execvp(argument_list[0] , argument_list);
	}

	FIFORequestChannel* constant_chan = new FIFORequestChannel("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(constant_chan);

	if(newChannel){
		//send newchannel request to the server
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
		constant_chan->cwrite(&nc, sizeof(MESSAGE_TYPE));

		//create a variable to hold the name
		char new_chan[30];
		//cread the response form the server
		constant_chan->cread(new_chan, sizeof(new_chan));
		string bruh = new_chan;
		cout << "howdy" << endl;
		//call the FIFORequestChannel constructor with the name from the server
		FIFORequestChannel* newChan = new FIFORequestChannel(bruh, FIFORequestChannel::CLIENT_SIDE);

		channels.push_back(newChan);
		cout << "howdy" << endl;
	}

	FIFORequestChannel chan = *(channels.back());
	cout << "howdy" << endl;

	if(p!=-1 && t!=-1 && e!=-1){
		// example data point request
		char buf[MAX_MESSAGE]; // 256
		// datamsg x(1, 0.0, 1); // change from hard code to user's values
		datamsg x(p, t, e);
		
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply;

	} else if (p!=-1 && t==-1 && e==-1){
		//make file
		ofstream of;
		string file_name = "received/x1.csv";

		of.open(file_name);

		//run through t from 0.004 to 1
		//run through e1 and e2
		char buf1[MAX_MESSAGE]; // 256
		char buf2[MAX_MESSAGE];


		double counter = 0;

		for(int i = 0; i < 1000; i++){
			datamsg x(p, counter, 1);
			datamsg y(p, counter, 2);

			memcpy(buf1, &x, sizeof(datamsg));
			memcpy(buf2, &y, sizeof(datamsg));

			//reading ecg 1
			chan.cwrite(buf1, sizeof(datamsg)); // question
			double reply1;
			chan.cread(&reply1, sizeof(double)); //answer

			//reading ecg 2
			chan.cwrite(buf2, sizeof(datamsg)); // question
			double reply2;
			chan.cread(&reply2, sizeof(double)); //answer

			of << counter << "," << reply1 << "," << reply2 << "\n";
			counter += 0.004;
		}	

		of.close();
	} else if(filename != ""){
		// sending a non-sense message, you need to change this
		filemsg fm(0, 0);

		string file = "received/" + filename;
		string fname = filename;
		cout << fname << endl;
		FILE* out = fopen(file.c_str(), "wb");
		
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf3 = new char[len];
		memcpy(buf3, &fm, sizeof(filemsg));
		strcpy(buf3 + sizeof(filemsg), fname.c_str());

		chan.cwrite(buf3, len);  // I want the file length;
		__int64_t filesize = 0;
		chan.cread(&filesize, sizeof(__int64_t)); //writing to file size

		char* buf4 = new char[m];
		__int64_t div = filesize / m;
		__int64_t rem = filesize % m;

		cout << filesize << endl;
		cout << div << endl;
		cout << rem << endl;

		//loop over segments in the (file filesize / buff capacity(m))
		filemsg* file_req = (filemsg*)buf3; //create filemsg instance

		file_req->offset = 0; //offset starts at 0
		file_req->length = m; //set length, careful of the last segment

		//loops until there is remainder
		for(__int64_t j = 0; j < div; j++){
			//send request (buf3)
			chan.cwrite(buf3, len);
			chan.cread(buf4, file_req->length);

			//receive the response
			//cread into buf4 length file_reg->length
			fwrite(buf4, 1, file_req->length, out);
			file_req->offset += m; //new offset
		}

		//remaining bytes set as length
		file_req->length = rem;
		chan.cwrite(buf3, len);
		chan.cread(buf4, file_req->length);
	

		fwrite(buf4, 1, file_req->length, out);
		fclose(out);

		delete[] buf3;
		delete[] buf4;
	}

	//if necessary, close and delete the new channel
	int chans = channels.size();

	for(int k = 0; k < chans; k++){
		// closing the channel    
    	MESSAGE_TYPE mes = QUIT_MSG;
    	chan.cwrite(&mes, sizeof(MESSAGE_TYPE));

		delete channels[k];	
	}
}

// your PA3 client code here

//change all instances of FIFOReqChan to TCPReqChan
//change to the appropriate constructor

//get opt, add a and r, collect values into vars


#include <fstream>
#include <iostream>
#include <thread>
#include <sys/time.h>
#include <sys/wait.h>

#include "BoundedBuffer.h"
#include "common.h"
#include "Histogram.h"
#include "HistogramCollection.h"
#include "TCPRequestChannel.h"

// Credit to Gabriel Stella for the passing data between the workers and the histogram threads
// and how to signal the histogram threads when to quit

// ecgno to use for datamsgs
#define EGCNO 1

// define a struct for patient, data pair
struct PatientData
{
    int patient;
    double data;
};

using namespace std;

void patient_thread_function(int n, int p, BoundedBuffer *request_buffer)
{
    datamsg data(p, 0.0, EGCNO);
    for (int i = 0; i < n; i++)
    {
        request_buffer->push(reinterpret_cast<char *>(&data), sizeof(datamsg));
        data.seconds += 0.004;
    }
}

void file_thread_function(TCPRequestChannel *chan, BoundedBuffer *request_buffer, string fname, int buf_size)
{
    // 1. Set up variables
    __uint64_t file_len = 0;
    __uint64_t remain_len = 0;

    // 2. Create the received file directory
    string f_dir = "received/" + fname;

    // 3. Open the file and make enough empty space
    //    as the original file

    // Getting the file size (remember PA1?)
    char buf[1024];
    filemsg f(0, 0);
    memcpy(buf, &f, sizeof(filemsg));
    strcpy(buf + sizeof(filemsg), fname.c_str());
    chan->cwrite(buf, sizeof(filemsg) + fname.size() + 1);
    chan->cread(&file_len, sizeof(file_len));

    // Open the file and move the cursor to the end of the file
    // (fseek)
    FILE *fp = fopen(f_dir.c_str(), "w");
    fseek(fp, file_len, SEEK_SET);
    fclose(fp);

    // 2. Generate all the file messages
    //    and push them into the bounded buffer
    filemsg *fm = reinterpret_cast<filemsg *>(buf);
    remain_len = file_len;

    while (remain_len > 0)
    {
        // 1. Update the filemsg length for each new messageg
        fm->length = min(buf_size, static_cast<int>(remain_len));

        // 2. Push the filemsg into the bounded buffer
        request_buffer->push(reinterpret_cast<char *>(fm), sizeof(filemsg) + fname.size() + 1);

        // 3. Update the remain_len
        fm->offset += fm->length;
        remain_len -= fm->length;
    }
}

void worker_thread_function(TCPRequestChannel *chan, BoundedBuffer *request_buffer, BoundedBuffer *response_buffer, int buf_size)
{
    // 1. Set up variable
    char buf[1024];
    double resp = 0;

    // 2. Read the file message from the bounded buffer
    while (true)
    {
        request_buffer->pop(buf, 1024);
        MESSAGE_TYPE *m = reinterpret_cast<MESSAGE_TYPE *>(buf);

        if (*m == DATA_MSG)
        {
            // Get patient's data from server
            chan->cwrite(buf, sizeof(datamsg));
            chan->cread(&resp, sizeof(double));

            // Pair the patient and his/her value together
            PatientData data = {(reinterpret_cast<datamsg *>(buf))->person, resp};

            // Convert pair values to bytes for
            // pushing to response buffer
            response_buffer->push(reinterpret_cast<char *>(&data), sizeof(data));
        }
        else if (*m == FILE_MSG)
        {
            // 1. Read the file message
            char *recv_buf = new char[buf_size];
            filemsg *fm = reinterpret_cast<filemsg *>(buf);
            string fname = reinterpret_cast<char *>(fm + 1);
            // printf("%s\n", fname.c_str());
            size_t sz = sizeof(filemsg) + fname.size() + 1;
            chan->cwrite(buf, sz);
            chan->cread(recv_buf, fm->length);

            // 2. Write the file message to the file
            string recv_fname = "received/" + fname;

            FILE *fp = fopen(recv_fname.c_str(), "r+");
            if (fp == nullptr)
            {
                cerr << "Error opening file" << endl;
                exit(1);
            }
            fseek(fp, fm->offset, SEEK_SET);
            fwrite(recv_buf, 1, fm->length, fp);
            fclose(fp);
            delete[] recv_buf;
        }
        else if (*m == QUIT_MSG)
        {
            chan->cwrite(m, sizeof(QUIT_MSG));
            delete chan;
            break;
        }
        else
        {
            cerr << "Unknown message type" << endl;
            exit(1);
        }
    }
}

void histogram_thread_function(BoundedBuffer *response_buffer, HistogramCollection *hc)
{
    // Keep popping data from the response buffer
    // and update the histogram, until receive
    // quit signals
    while (true)
    {
        PatientData data;
        response_buffer->pop(reinterpret_cast<char *>(&data), sizeof(data));
        if (data.patient < 0) // quit signal detected
            break;
        hc->update(data.patient, data.data);
    }
}

// TCPRequestChannel *create_new_channel(TCPRequestChannel *chan)
// {
//     char name[1024];
//     MESSAGE_TYPE m = NEWCHANNEL_MSG;
//     chan->cwrite(&m, sizeof(m));
//     chan->cread(name, 1024);
//     TCPRequestChannel *new_chan = new TCPRequestChannel(name, TCPRequestChannel::CLIENT_SIDE);
//     return new_chan;
// }

int main(int argc, char *argv[])
{
    int n = 1000;        // default number of requests per "patient"
    int p = 10;          // number of patients [1,15]
    int w = 100;         // default number of worker threads
    int h = 20;          // default number of histogram threads
    int b = 20;          // default capacity of the request buffer (should be changed)
    int m = MAX_MESSAGE; // default capacity of the message buffer
    string f = "";       // name of file to be transferred
    string host;
    string port;

    // read arguments
    int opt;
    while ((opt = getopt(argc, argv, "a:r:n:p:w:h:b:m:f:")) != -1)
    {
        switch (opt)
        {
        case 'n':
            n = atoi(optarg);
            break;
        case 'p':
            p = atoi(optarg);
            break;
        case 'w':
            w = atoi(optarg);
            break;
        case 'h':
            h = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 'm':
            m = atoi(optarg);
            break;
        case 'f':
            f = optarg;
            break;
        case 'a':
            host = optarg;
            break;
        case 'r':
            port = optarg;
            break;
        }
    }

    // fork and exec the server
    // int pid = fork();
    // if (pid == 0)
    // {
    //     execl("./server", "./server", "-m", (char *)to_string(m).c_str(), nullptr);
    // }

    // initialize overhead (including the control channel)
    TCPRequestChannel *chan = new TCPRequestChannel(host, port);
    BoundedBuffer request_buffer(b);
    BoundedBuffer response_buffer(b);
    HistogramCollection hc;
    vector<TCPRequestChannel *> worker_channels;

    // making histograms and adding to collection
    for (int i = 0; i < p; i++)
    {
        Histogram *h = new Histogram(10, -2.0, 2.0);
        hc.add(h);
    }

    // making worker channels
    for (int i = 0; i < w; i++)
    {
        // TCPRequestChannel *new_chan = create_new_channel(chan);
        TCPRequestChannel *new_chan = new TCPRequestChannel(host, port);
        worker_channels.push_back(new_chan);
    }

    // record start time
    struct timeval start, end;
    gettimeofday(&start, 0);

    /* create all threads here */
    // 1. Create patient, file, and worker threads
    thread *patient_threads = new thread[p];
    thread *histogram_threads = new thread[h];
    thread file_thread;

    if (!f.empty())
        file_thread = thread(file_thread_function, chan, &request_buffer, f, m);
    else
    {
        for (int i = 0; i < p; i++)
            patient_threads[i] = thread(patient_thread_function, n, i + 1, &request_buffer);
        for (int i = 0; i < h; i++)
            histogram_threads[i] = thread(histogram_thread_function, &response_buffer, &hc);
    }

    thread *worker_threads = new thread[w];
    for (int i = 0; i < w; i++)
    {
        worker_threads[i] = thread(worker_thread_function, worker_channels[i], &request_buffer, &response_buffer, m);
    }

    /* join all threads here */
    if (!f.empty())
    {
        file_thread.join();
    }
    else
    {
        for (int i = 0; i < p; i++)
            patient_threads[i].join();
    }

    for (int i = 0; i < w; ++i)
    {
        MESSAGE_TYPE q = QUIT_MSG;
        request_buffer.push(reinterpret_cast<char *>(&q), sizeof(q));
    }

    // join worker threads
    for (int i = 0; i < w; i++)
    {
        worker_threads[i].join();
    }

    // push quit signals to response buffer for
    // histogram threads and join them together
    if (f.empty())
    {
        PatientData quit_sig = {-1, -1};
        for (int i = 0; i < h; i++)
            response_buffer.push(reinterpret_cast<char *>(&quit_sig), sizeof(quit_sig));

        for (int i = 0; i < h; i++)
            histogram_threads[i].join();
    }

    // record end time
    gettimeofday(&end, 0);

    // print the results
    if (f == "")
    {
        hc.print();
    }
    int secs = ((1e6 * end.tv_sec - 1e6 * start.tv_sec) + (end.tv_usec - start.tv_usec)) / ((int)1e6);
    int usecs = (int)((1e6 * end.tv_sec - 1e6 * start.tv_sec) + (end.tv_usec - start.tv_usec)) % ((int)1e6);
    cout << "Took " << secs << " seconds and " << usecs << " micro seconds" << endl;

    // quit and close control channel
    MESSAGE_TYPE q = QUIT_MSG;
    chan->cwrite((char *)&q, sizeof(MESSAGE_TYPE));
    cout << "All Done!" << endl;

    // wait for server to exit
    // wait(nullptr);

    // clean up
    delete[] patient_threads;
    delete[] histogram_threads;
    delete[] worker_threads;
    delete chan;

    return 0;
}
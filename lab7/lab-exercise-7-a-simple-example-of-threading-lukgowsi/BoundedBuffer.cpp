#include "BoundedBuffer.h"


using namespace std;


BoundedBuffer::BoundedBuffer (int _cap) : cap(_cap) {
    // modify as needed
}

BoundedBuffer::~BoundedBuffer () {
    // modify as needed
}

void BoundedBuffer::push (char* msg, int size) {
    // 1. Convert the incoming byte sequence given by msg and size into a vector<char>
    // 2. Wait until there is room in the queue (i.e., queue lengh is less than cap)
    // 3. Then push the vector at the end of the queue
    // 4. Wake up threads that were waiting for push

    vector<char> data(msg, msg + size);

    unique_lock<mutex> locker(m);
    slotAvailable.wait(locker, [this]{return ((size_t)q.size() < (size_t)cap);});

    q.push(data);

    locker.unlock();
    dataAvailable.notify_one();
}

int BoundedBuffer::pop (char* msg, int size) {
    // 1. Wait until the queue has at least 1 item
    // 2. Pop the front item of the queue. The popped item is a vector<char>
    // 3. Convert the popped vector<char> into a char*, copy that into msg; assert that the vector<char>'s length is <= size
    // 4. Wake up threads that were waiting for pop
    // 5. Return the vector's length to the caller so that they know how many bytes were popped

    unique_lock<mutex> locker(m);
    dataAvailable.wait(locker, [this]{return ((size_t)q.size() >= (size_t)1);});

    vector<char> thing = q.front();
    string s;

    memcpy(msg, thing.data(), thing.size());
    q.pop();

    locker.unlock();
    slotAvailable.notify_one();

    return s.size();
}

size_t BoundedBuffer::size () {
    return q.size();
}
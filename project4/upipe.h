struct upipe {
    int read_index; // where in the read part of the pipe this is
    int write_index; // where in the write part of the pipe this is
    int count; // # total bytes in pipe
    int size; // size of data available inside the buffer
    char data[512]; // space for the data to go to
};

struct upipe *upipe_create(int size, void *vaddr);
struct upipe *upipe_attach(void *vaddr);
int upipe_detach(struct upipe *p);
int upipe_write(struct upipe *p, char *buf, int n);
int upipe_read(struct upipe * p, char *buf, int n);

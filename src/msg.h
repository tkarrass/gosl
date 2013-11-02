
struct Buffer {
	int size;
	void *data;
};

struct message {
	uint32_t timestamp;
	uint32_t width;		// varies
	uint32_t height;	// normally 80
	//char **image;	// dimension is width x height
	char *image;	// dimension is width x height
};

int getBufferSize(struct message *msg);
void serialize(char *buf,struct message *msg);
void deserialize(struct message *msg,const char *buf);

struct Buffer *new_buffer();
void append_space(Buffer *b,int n);
void serialize_int(int x,Buffer *b);
void serialize_string(char *str,Buffer *b);
void serialize_message(struct message *msg,Buffer *b);


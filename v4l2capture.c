#include<stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define VIDEO_DEVICE "/dev/video0"  // Change this to your video device
#define IMAGE_WIDTH 10  // Change this to your desired width
#define IMAGE_HEIGHT 10  // Change this to your desired height
#define OUTPUT_IMAGE "image.jpg"  // Output image file name

struct buffer {
    void* start;
    size_t length;
};

int main() {
    // Open the video device
    int fd = open(VIDEO_DEVICE, O_RDWR);
    if (fd == -1) {
        perror("Failed to open video device");
        return 1;
    }

    // Set the video format
    struct v4l2_format format;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = IMAGE_WIDTH;
    format.fmt.pix.height = IMAGE_HEIGHT;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    format.fmt.pix.field = V4L2_FIELD_NONE;
    if (ioctl(fd, VIDIOC_S_FMT, &format) == -1) {
        perror("Failed to set video format");
        close(fd);
        return 1;
    }
   // Request a single capture buffer
    struct v4l2_requestbuffers reqbuf;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    reqbuf.count = 10;
    if (ioctl(fd, VIDIOC_REQBUFS, &reqbuf) == -1) {
        perror("Failed to request buffer");
        close(fd);
        return 1;
    }

    // Map the capture buffer
    struct buffer buffer;
    for(int i = 0;i<10;i++) {
    buffer.length = reqbuf.count;
    buffer.start = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (buffer.start == MAP_FAILED) {
        perror("Failed to map capture buffer");
        close(fd);
        return 1;
    }

    // Queue the capture buffer
    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
        perror("Failed to query buffer");
        munmap(buffer.start, buffer.length);
        close(fd);
        return 1;
    }

    // Queue the buffer for capture
      if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
        perror("Failed to queue buffer");
        munmap(buffer.start, buffer.length);
        close(fd);
        return 1;
    }

    // Start capturing
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        perror("Failed to start capturing");
        munmap(buffer.start, buffer.length);
        close(fd);
        return 1;
    }

    // Wait for capture
    if (ioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        perror("Failed to dequeue buffer");
        munmap(buffer.start, buffer.length);
        close(fd);
        return 1;
    }

    // Save captured image to file
    FILE* outfile = fopen(OUTPUT_IMAGE, "wb");
    if (outfile == NULL) {
        perror("Failed to open output file");
        munmap(buffer.start, buffer.length);
        close(fd);
        return 1;
    }
    fwrite(buffer.start, buf.bytesused, 1, outfile);

    fclose(outfile);
     }
    // Cleanup
    munmap(buffer.start, buffer.length);
    close(fd);

    return 0;
    }

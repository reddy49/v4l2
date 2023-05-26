#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define VIDEO_DEVICE "/dev/video0"
#define VIDEO_WIDTH 640
#define VIDEO_HEIGHT 480
#define VIDEO_FORMAT V4L2_PIX_FMT_YUYV
#define NUM_BUFFERS 4

struct buffer {
    void *start;
    size_t length;
};

int main() {
    int video_fd;
    struct v4l2_format format;
    struct v4l2_requestbuffers reqbuf;
    struct buffer *buffers;
    int i;

    // Open the video device
    video_fd = open(VIDEO_DEVICE, O_RDWR);
    if (video_fd == -1) {
        perror("Failed to open video device");
        return 1;
    }

    // Set video format
    memset(&format, 0, sizeof(format));
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = VIDEO_WIDTH;
    format.fmt.pix.height = VIDEO_HEIGHT;
    format.fmt.pix.pixelformat = VIDEO_FORMAT;
    if (ioctl(video_fd, VIDIOC_S_FMT, &format) == -1) {
        perror("Failed to set video format");
        close(video_fd);
        return 1;
    }

    // Request video buffers
    memset(&reqbuf, 0, sizeof(reqbuf));
    reqbuf.count = NUM_BUFFERS;
    reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    reqbuf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(video_fd, VIDIOC_REQBUFS, &reqbuf) == -1) {
        perror("Failed to request video buffers");
        close(video_fd);
        return 1;
    }

    // Allocate buffer structures
    buffers = calloc(reqbuf.count, sizeof(*buffers));
    if (buffers == NULL) {
        perror("Failed to allocate buffer structures");
        close(video_fd);
        return 1;
    }

    // Map video buffers
    for (i = 0; i < reqbuf.count; i++) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(video_fd, VIDIOC_QUERYBUF, &buf) == -1) {
            perror("Failed to query video buffer");
            close(video_fd);
            return 1;
        }

        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, buf.m.offset);
        if (buffers[i].start == MAP_FAILED) {
            perror("Failed to map video buffer");
            close(video_fd);
            return 1;
        }
    }

    // Start capturing
    for (i = 0; i < reqbuf.count; i++) {
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(video_fd, VIDIOC_QBUF, &buf) == -1) {
            perror("Failed to enqueue video buffer");
            close(video_fd);
            return 1;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(video_fd, VIDIOC_STREAMON, &type) == -1) {
        perror("Failed to start video streaming");
        close(video_fd);
        return 1;
    }

    // Capture video frames
    for (i = 0; i < 100; i++) {  // Capture 100 frames
        struct v4l2_buffer buf;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(video_fd, VIDIOC_DQBUF, &buf) == -1) {
            perror("Failed to dequeue video buffer");
            close(video_fd);
            return 1;
        }

        // Store the captured frame as an image file
        char filename[20];
        snprintf(filename, sizeof(filename), "frame%d.jpg", i);
        FILE *image_file = fopen(filename, "wb");
        if (image_file == NULL) {
            perror("Failed to open image file");
            close(video_fd);
            return 1;
        }
        fwrite(buffers[buf.index].start, buf.bytesused, 1, image_file);
        fclose(image_file);

        if (ioctl(video_fd, VIDIOC_QBUF, &buf) == -1) {
            perror("Failed to enqueue video buffer");
            close(video_fd);
            return 1;
        }
    }

    // Stop capturing
    if (ioctl(video_fd, VIDIOC_STREAMOFF, &type) == -1) {
        perror("Failed to stop video streaming");
        close(video_fd);
        return 1;
    }

    // Clean up
    for (i = 0; i < reqbuf.count; i++) {
        munmap(buffers[i].start, buffers[i].length);
    }
    free(buffers);
    close(video_fd);

    return 0;
}

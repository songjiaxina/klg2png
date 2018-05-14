#include <iostream>
#include <stdio.h>
#include <cassert>
#include <cassert>
#include <zlib.h>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <opencv2/opencv.hpp>
#include "jpeglib.h"
#include <stack>
////zlib(http://zlib.net/)提供了简洁高效的In-Memory数据压缩和解压缩系列API函数，
/// 很多应用都会用到这个库，其中compress和uncompress函数是最基本也是最常用的
using namespace std;
static void jpegFail(j_common_ptr cinfo)
{
    assert(false && "JPEG decoding error!");
}

static void doNothing(j_decompress_ptr)
{

}

class JPEGLoader
{
public:
    JPEGLoader()
    {}

    void readData(unsigned char * src, const int numBytes, unsigned char * data)
    {
        jpeg_decompress_struct cinfo; // IJG JPEG codec structure

        jpeg_error_mgr errorMgr;

        errorMgr.error_exit = jpegFail;

        cinfo.err = jpeg_std_error(&errorMgr);

        jpeg_create_decompress(&cinfo);

        jpeg_source_mgr srcMgr;

        cinfo.src = &srcMgr;

        // Prepare for suspending reader
        srcMgr.init_source = doNothing;
        srcMgr.resync_to_restart = jpeg_resync_to_restart;
        srcMgr.term_source = doNothing;
        srcMgr.next_input_byte = src;
        srcMgr.bytes_in_buffer = numBytes;

        jpeg_read_header(&cinfo, TRUE);

        jpeg_calc_output_dimensions(&cinfo);

        jpeg_start_decompress(&cinfo);

        int width = cinfo.output_width;
        int height = cinfo.output_height;

        JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, width * 4, 1);

        for(; height--; data += (width * 3))
        {
            jpeg_read_scanlines(&cinfo, buffer, 1);

            unsigned char * bgr = (unsigned char *)buffer[0];
            unsigned char * rgb = (unsigned char *)data;

            for(int i = 0; i < width; i++, bgr += 3, rgb += 3)
            {
                unsigned char t0 = bgr[0], t1 = bgr[1], t2 = bgr[2];
                rgb[2] = t0; rgb[1] = t1; rgb[0] = t2;
            }
        }

        jpeg_finish_decompress(&cinfo);

        jpeg_destroy_decompress(&cinfo);
    }
};
class FileReader
{
public:
    FileReader(std::string filename_,int width_,int height_, std::string output_rgb_dir_,
               std::string output_depth_dir_, bool visible_=true,bool iswrite_=true):
            filename(filename_),width(width_),height(height_), visible(visible_),iswrite(iswrite_),
            output_rgb_dir(output_rgb_dir_),output_depth_dir(output_depth_dir_)
    {
        mjpegloader=new JPEGLoader();
        numPixels=width*height;
        fp=fopen(filename.c_str(), "rb");
        num_frame=0;
        auto tmp=fread(&numFrames,sizeof(int32_t),1,fp);
        assert(tmp);
        mvnumframes.push_back(numFrames);
        cout<<"已经读取的 numFrames: "<<numFrames<<endl;

    }
    ~FileReader()
    {
        delete mjpegloader;
        fclose(fp);
    }

    void process_one_frame();
    void create_filetxt();
    std::string filename;
    std::string output_rgb_dir;
    std::string output_depth_dir;

    std::vector<int64_t >mvtimestamps;
    std::vector<int32_t >mvnumframes;

    int numPixels;
    int width;
    int height;
    int num_frame;
    int32_t numFrames;

    bool visible;
    bool iswrite;

    JPEGLoader* mjpegloader;

    FILE * fp;

};

using namespace std;

int main(int argc,char** argv)
{


    const std::string file=argv[1];
    const std::string outputrgb="/home/dl/rgb/";
    const std::string outputdep="/home/dl/dep/";

    ////the last two parameter stands for whether is visible and is write
    FileReader* readfile=new FileReader(file,640,480,outputrgb,outputdep,true,false);


    for(int i=0;i<readfile->numFrames;i++)
    {
        readfile->process_one_frame();
    }

    //readfile->create_filetxt();



    return 0;
}

void FileReader::process_one_frame()
{
    //// Bytef其实就是unsigened char*

    ////depthReadBuffer为深度图片读取缓冲器,大小为像素数乘以2
    ////imageReadBuffer为彩色图片读取缓冲器,大小为像素数乘以3
    unsigned char * depthReadBuffer=new unsigned char[numPixels * 2];
    unsigned char * imageReadBuffer=new unsigned char[numPixels * 3];

    ////压缩的深度图片读取文件缓冲器
    ////压缩的彩色图片读取文件缓冲器
    Bytef * decompressionBufferDepth=new Bytef[numPixels * 2];
    Bytef * decompressionBufferImage=new Bytef[numPixels * 3];


    int32_t depthSize;
    int32_t imageSize;
    int64_t timestamp;


    auto tmp = fread(&timestamp,sizeof(int64_t),1,fp);
    assert(tmp);
    mvtimestamps.push_back(timestamp);
    std::cout<<"已经读取的timestamp: "<<timestamp<<std::endl;


    tmp = fread(&depthSize,sizeof(int32_t),1,fp);
    assert(tmp);
    std::cout<<"depthsize: "<<depthSize<<std::endl;

    tmp = fread(&imageSize,sizeof(int32_t),1,fp);
    assert(tmp);
    std::cout<<"imagesize: "<<imageSize<<std::endl;


    tmp = fread(depthReadBuffer,depthSize,1,fp);
    assert(tmp);

    tmp = fread(imageReadBuffer,imageSize,1,fp);
    assert(tmp);

    unsigned long decompLength = numPixels * 2;
    if(uncompress(&decompressionBufferDepth[0], (unsigned long *)&decompLength,
                  (const Bytef *)depthReadBuffer, depthSize)!= Z_OK)
    {
        std::cerr<<"转换文件错误"<<std::endl;
        return ;
    }

    mjpegloader->readData(imageReadBuffer, imageSize, (unsigned char *) &decompressionBufferImage[0]);


    unsigned short* depth = (unsigned short *)decompressionBufferDepth;
    unsigned char* rgb = (unsigned char *)&decompressionBufferImage[0];

    cv::Mat rgb_image(height,width,CV_8UC3);

    //rgb_image.data=rgb;
    ///this can not change the channels rgb bgr


    cv::Mat_<cv::Vec3b>::iterator it=rgb_image.begin<cv::Vec3b>();
    cv::Mat_<cv::Vec3b>::iterator itend=rgb_image.end<cv::Vec3b>();
    int index=0;
    for(;it !=itend;it++)
    {
        (*it)[2]=rgb[index];
        index++;
        (*it)[1]=rgb[index];
        index++;
        (*it)[0]=rgb[index];
        index++;
    }

    if(visible)
    {
        cv::imshow("生成的rgb图像",rgb_image);
        cv::waitKey(1);
    }
    if(iswrite)
    {
        std::string write_filename_rgb=output_rgb_dir+std::to_string(timestamp)+"_rgb.png";
        std::cout<<"filename: "<<write_filename_rgb<<std::endl;
        cv::imwrite(write_filename_rgb,rgb_image);
    }


    cv::Mat depth_image(height,width,CV_16UC1);
    int index2=0;
    for(int v=0;v<height;v++)
    {
        for(int u=0;u<width;u++)
        {
            depth_image.ptr<unsigned short>(v)[u]=depth[index2];
            index2++;
        }
    }
    if(visible)
    {
        cv::imshow("生成的depth图像",depth_image);
        cv::waitKey(1);
    }

    if(iswrite)
    {
        std::string write_filename_depth=output_depth_dir+std::to_string(timestamp)+"_depth.png";
        cv::imwrite(write_filename_depth,depth_image);
    }


    num_frame++;
    std::cout<<"处理了"<<num_frame<<"帧/共"<<numFrames<<"帧"<<std::endl;

    delete [] depthReadBuffer;
    delete [] imageReadBuffer;
    delete [] decompressionBufferDepth;
    delete [] decompressionBufferImage;
}
void FileReader::create_filetxt()
{
    ofstream fin("/home/dl/index.txt");
    for(int i=0;i<mvtimestamps.size();i++)
    {
        fin<<mvtimestamps[i]<<endl;
    }
    fin.close();

}
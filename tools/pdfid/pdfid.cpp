#include <podofo-base.h>

#include <memory>
#include <iostream>
#include <fstream>
#include <string>

using std::cerr;
using std::cout;
using std::cin;
using std::flush;
using std::endl;
using std::string;

using namespace PoDoFo;

enum PdfIDMode
{
    PDFID_READ,
    PDFID_WRITE,
};

struct PdfIDOptions
{
    EPdfWriteMode eWriteMode;
    bool useDemandLoading;
    bool useStrictMode;
    const char* input_filename;
    const char* output_filename;
    const char* input_data;
    const char* output_data;
    enum PdfIDMode mode;

    PdfIDOptions()
        : eWriteMode(ePdfWriteMode_Default)
        , useDemandLoading(false)
        , useStrictMode(false)
        , input_filename(nullptr)
        , output_filename(nullptr)
        , input_data(nullptr)
        , output_data(nullptr)
        , mode(PDFID_READ)
    {}

    static int usage(int rc)
    {
        cerr << ("Usage: pdfid [-d] [-s] [-clean] [-compact] [-r <output_data>]"
                 " [-w <input_data>] <input_filename> <output_filename>\n")
             << "    -d       Enable demand loading of objects\n"
             << "    -s       Enable strict parsing mode\n"
             << "    -clean   Enable clean writing mode\n"
             << "    -compact Enable compact writing mode\n"
             << "    -r       Read data from the pdf file into output_data\n"
             << "    -w       Write input_data into the pdf file\n"
             << flush;
        return rc;
    }

    // worst argument parsing ever
    int parse(int argc, char *argv[])
    {
        for (int i = 1; i < argc; i++)
        {
            if (argv[i][0] == '-')
            {
                if (string("-d") == argv[i])
                    useDemandLoading = true;
                else if (string("-s") == argv[1])
                    useStrictMode = true;
                else if (string("-clean") == argv[i])
                    eWriteMode = ePdfWriteMode_Clean;
                else if (string("-compact") == argv[i])
                    eWriteMode = ePdfWriteMode_Compact;
                else if (string("-r") == argv[i])
                {
                    mode = PDFID_READ;
                    if ((output_data = argv[i++ + 1]) == nullptr)
                    {
                        std::cerr << "Missing input file" << std::endl;
                        return usage(1);
                    }
                }
                else if (string("-w") == argv[i])
                {
                    mode = PDFID_WRITE;
                    if ((input_data = argv[i++ + 1]) == nullptr)
                    {
                        std::cerr << "Missing output file" << std::endl;
                        return usage(1);
                    }

                }
                continue;
            }

            if (input_filename == NULL)
                input_filename = argv[i];
            else if (output_filename == NULL)
                output_filename = argv[i];
            else
            {
                cerr << "unknown positional argument: " << argv[i] << endl;
                return 1;
            }
        }

        switch (mode)
        {
        case PDFID_READ:
            if (input_filename != nullptr && output_filename == nullptr && output_data != nullptr)
                return 0;
            break;
        case PDFID_WRITE:
            if (input_filename != nullptr && output_filename != nullptr)
                return 0;
            break;
        }
        return usage(1);
    }
};

int pdfid_read(PdfParser &parser, PdfIDOptions &options)
{
    std::ofstream output_data_stream;
    try {
        output_data_stream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        output_data_stream.open(options.output_data, std::ofstream::out | std::ofstream::trunc);
    } catch (std::ofstream::failure e) {
        std::cerr << "failed to open file: " << e.what() << std::endl;
        return 1;
    }

    bit_ostream o_bitstream(output_data_stream);
    parser.SetStrictParsing(options.useStrictMode);
    PdfRefCountedInputDevice device( options.input_filename, "rb" );
    if( !device.Device() )
    {
        std::cerr << "Couldn't open input file" << std::endl;
        return 1;
    }
    device.Device()->dictdecode_stream = &o_bitstream;
    parser.ParseFile(device, options.useDemandLoading);
    o_bitstream.flush();
    return 0;
}

int pdfid_write(PdfParser &parser, PdfIDOptions &options)
{
    // setup the input data stream
    std::ifstream input_data_stream;
    input_data_stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        input_data_stream.open(options.input_data, std::ifstream::in);
        input_data_stream.exceptions(std::ifstream::badbit);
    } catch (std::ifstream::failure e) {
        std::cerr << "failed to open file: " << e.what() << std::endl;
        return 1;
    }
    bit_istream i_bitstream(input_data_stream);

    // read the file
    parser.SetStrictParsing(options.useStrictMode);
    parser.ParseFile(options.input_filename, options.useDemandLoading);

    // write it back
    PdfWriter writer( &parser );
    writer.SetWriteMode( options.eWriteMode );
    writer.SetPdfVersion( ePdfVersion_1_6 );
    PdfOutputDevice device( options.output_filename );
    device.dictencode_stream = &i_bitstream;
    writer.Write( &device );
    return 0;
}

int main( int argc, char*  argv[] )
{
    int rc;

    PdfIDOptions options;
    if ((rc = options.parse(argc, argv)))
        return rc;

    PdfError::EnableLogging(true);
    PdfError::EnableDebug(true);

    PdfVecObjects objects;
    PdfParser parser(&objects);

    objects.SetAutoDelete(true);

    try {
        switch (options.mode)
        {
        case PDFID_READ:
            return pdfid_read(parser, options);
        case PDFID_WRITE:
            return pdfid_write(parser, options);
        }
    } catch (PdfError & e) {
        e.PrintErrorMsg();
        return e.GetError();
    }

    cerr << "Parsed and wrote successfully" << endl;
    return 0;
}

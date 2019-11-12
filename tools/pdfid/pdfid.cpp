#include <podofo-base.h>

#include <memory>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>

using namespace PoDoFo;

int open_output_file(std::ofstream &file, const char *path)
{
    try {
        file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        file.open(path, std::ofstream::out | std::ofstream::trunc);
        return 0;
    } catch (const std::ofstream::failure &e) {
        std::cerr << "failed to open file `"
                  << path << "': "
                  << strerror(errno) << std::endl;
        return 1;
    }
}

int open_input_file(std::ifstream &file, const char *path)
{
    file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try {
        file.open(path, std::ifstream::in);
        file.exceptions(std::ifstream::badbit);
        return 0;
    } catch (const std::ifstream::failure &e) {
        std::cerr << "failed to open file `"
                  << path << "': "
                  << strerror(errno) << std::endl;
        return 1;
    }
}

void pdfid_read_help(const char *program_name, std::ostream &o)
{
    o << "Usage: " << program_name
      << " read <input_pdf> [<output_data_file>]" << std::endl;
}

int pdfid_read_stream(PdfParser &parser, std::ostream &output_data_stream, const char *input_filename)
{
    int rc;

    bit_ostream o_bitstream(output_data_stream);

    std::ifstream input_file_stream;
    if ((rc = open_input_file(input_file_stream, input_filename)))
        return rc;

    PdfRefCountedInputDevice device(new PdfInputDevice(&input_file_stream));
    device.Device()->dictdecode_stream = &o_bitstream;
    try {
        parser.ParseFile(device, false);
    } catch (const PdfError &e) {
        e.PrintErrorMsg();
        return 1;
    }
    o_bitstream.flush();
    return 0;
}

int pdfid_read(PdfParser &parser, const char *program_name, int argc, char *argv[])
{
    int rc;

    if (argc != 2 && argc != 3) {
        pdfid_read_help(program_name, std::cerr);
        return 1;
    }

    const char *input_file = argv[1];
    const char *output_data_file = argv[2];

    // if there's no output file, write to stdout
    if (argc == 2)
        return pdfid_read_stream(parser, std::cout, input_file);

    std::ofstream output_data_stream;
    if ((rc = open_output_file(output_data_stream, output_data_file)))
        return rc;

    return pdfid_read_stream(parser, output_data_stream, input_file);
}

void pdfid_write_help(const char *program_name, std::ostream &o)
{
    o << "Usage: " << program_name
      << " write <input_pdf> <output_pdf> [<input_data>]" << std::endl;
}

int pdfid_write_stream(PdfParser &parser, std::istream &input_data_stream, const char *input_pdf_path, const char *output_pdf_path)
{
    int rc;

    // open the input pdf file
    std::ifstream input_file_stream;
    if ((rc = open_input_file(input_file_stream, input_pdf_path)))
        return rc;

    // setup the input device
    PdfRefCountedInputDevice input_device(new PdfInputDevice(&input_file_stream));

    // read the file
    try {
        parser.ParseFile(input_device, false);
    } catch (const PdfError &e) {
        e.PrintErrorMsg();
        return 1;
    }

    // setup the output parser
    PdfWriter writer(&parser);
    writer.SetWriteMode(ePdfWriteMode_Compact);
    writer.SetPdfVersion(ePdfVersion_1_6);

    // setup the output device
    std::ofstream output_pdf_stream;
    if ((rc = open_output_file(output_pdf_stream, output_pdf_path)))
        return rc;
    bit_istream i_bitstream(&input_data_stream);
    PdfOutputDevice device(&output_pdf_stream);
    device.dictencode_stream = &i_bitstream;

    // write the pdf file
    writer.Write(&device);

    // if all input data was consumed, the write succeeded
    if (i_bitstream.eof())
        return 0;

    std::cerr << "The PDF file doesn't have sufficient capacity to "
        "hold all given data." << std::endl;
    std::cerr << "The file can hold at most " <<
        i_bitstream.bit_size / 8 << " hidden bytes " << std::endl;
    return 2;
}


int pdfid_write(PdfParser &parser, const char *program_name, int argc, char *argv[])
{
    int rc;

    if (argc != 3 && argc != 4) {
        pdfid_write_help(program_name, std::cerr);
        return 1;
    }

    const char *input_pdf_path = argv[1];
    const char *output_pdf_path = argv[2];
    const char *input_data_path = argv[3];

    if (argc == 3)
        return pdfid_write_stream(parser, std::cin, input_pdf_path, output_pdf_path);

    // setup the input data stream
    std::ifstream input_data_stream;
    if ((rc = open_input_file(input_data_stream, input_data_path)))
        return rc;

    return pdfid_write_stream(parser, input_data_stream, input_pdf_path, output_pdf_path);
}

void pdfid_capacity_help(const char *program_name, std::ostream &o)
{
    o << "Usage: " << program_name << " capacity <input_pdf>" << std::endl;
}

int pdfid_capacity(PdfParser &parser, const char *program_name, int argc, char *argv[])
{
    int rc;

    if (argc != 2) {
        pdfid_capacity_help(program_name, std::cerr);
        return 1;
    }

    std::ifstream input_file_stream;
    if ((rc = open_input_file(input_file_stream, argv[1])))
        return rc;

    PdfRefCountedInputDevice input_device(new PdfInputDevice(&input_file_stream));
    try {
        parser.ParseFile(input_device, false);
    } catch (const PdfError &e) {
        e.PrintErrorMsg();
        return 1;
    }

    // setup the output parser
    PdfWriter writer(&parser);
    writer.SetWriteMode(ePdfWriteMode_Compact);
    writer.SetPdfVersion(ePdfVersion_1_6);

    // setup the output device
    bit_istream i_bitstream(nullptr);
    PdfOutputDevice output_device;
    output_device.dictencode_stream = &i_bitstream;

    // write the pdf file
    writer.Write(&output_device);

    // the capacity should be displayed in bytes, as the API doesn't
    // have a bit-level granualarity anyway
    std::cout << i_bitstream.bit_size / 8 << std::endl;
    return 0;
}

struct PdfIDSubcommand {
    const char *name;
    int (*command)(PdfParser &parser, const char *program_name, int argc, char *argv[]);
    void (*help)(const char *program_name, std::ostream &o);
};

PdfIDSubcommand subcommands[] = {
    {
        .name = "write",
        .command = pdfid_write,
        .help = pdfid_write_help
    },
    {
        .name = "read",
        .command = pdfid_read,
        .help = pdfid_read_help
    },
    {
        .name = "capacity",
        .command = pdfid_capacity,
        .help = pdfid_capacity_help
    },
};

void help(const char *program_name, std::ostream &o)
{
    for (const auto& subcommand : subcommands)
        subcommand.help(program_name, o);
}

int main(int argc, char* argv[])
{
    PdfError::EnableLogging(true);
    PdfError::EnableDebug(true);

    PdfVecObjects objects;
    PdfParser parser(&objects);
    parser.SetStrictParsing(false);
    objects.SetAutoDelete(true);

    if (argc < 2) {
        help(argv[0], std::cerr);
        return 1;
    }

    argc -= 1;
    argv += 1;

    const char *subcommand_name = argv[0];
    for (const auto& subcommand : subcommands)
        if (strcmp(subcommand.name, subcommand_name) == 0)
            return subcommand.command(parser, argv[0], argc, argv);

    help(argv[0], std::cerr);
    return 1;
}

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
    parser.SetStrictParsing(false);

    std::ifstream input_file_stream;
    if ((rc = open_input_file(input_file_stream, input_filename)))
        return rc;

    PdfRefCountedInputDevice device(new PdfInputDevice(&input_file_stream));
    device.Device()->dictdecode_stream = &o_bitstream;
    parser.ParseFile(device, false);
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

    bit_istream i_bitstream(input_data_stream);

    // read the file
    parser.SetStrictParsing(false);
    parser.ParseFile(input_pdf_path, false);

    // write it back
    PdfWriter writer(&parser);
    writer.SetWriteMode(ePdfWriteMode_Compact);
    writer.SetPdfVersion(ePdfVersion_1_6);

    std::ofstream output_pdf_stream;
    if ((rc = open_output_file(output_pdf_stream, output_pdf_path)))
        return rc;

    PdfOutputDevice device(&output_pdf_stream);
    device.dictencode_stream = &i_bitstream;
    writer.Write(&device);
    return 0;
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

struct PdfIDSubcommand {
    int (*command)(PdfParser &parser, const char *program_name, int argc, char *argv[]);
    void (*help)(const char *program_name, std::ostream &o);
    const char *name;
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
};

void help(const char *program_name, std::ostream &o)
{
    for (const auto& subcommand : subcommands)
        subcommand.help(program_name, o);
}

int main(int argc, char* argv[])
{
    int rc;

    PdfError::EnableLogging(true);
    PdfError::EnableDebug(true);

    PdfVecObjects objects;
    PdfParser parser(&objects);
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

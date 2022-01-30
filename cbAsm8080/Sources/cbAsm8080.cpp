//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// $BeginLicense$
//
// $EndLicense$
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "cbUnit.h"

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

cbUnit  Unit;
bool    GlobalError = false;
int     Strict = 0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#define C_STRING(X) X.toLocal8Bit().data()

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

QString OutFileName(QString InFileName, QString OutDirName, QString Suffix)
    {
    QFileInfo FileInfo(InFileName);
    QString   OutBaseName = FileInfo.baseName() + "." + Suffix;
    if (OutDirName.size())
        {
        return QDir(OutDirName).filePath(OutBaseName);
        }
    else
        {
        return FileInfo.dir().filePath(OutBaseName);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void CustomMessageHandler(const QtMsgType           Type, 
                          const QMessageLogContext& Context,
                          const QString&            Message) 
    {
    QString Severity;
    switch (Type)
        {
        case QtDebugMsg    : Severity = QObject::tr("Debug");    break;
        case QtInfoMsg     : Severity = QObject::tr("Info");     break;
        case QtWarningMsg  : Severity = QObject::tr("Warning");  break;
        case QtCriticalMsg : Severity = QObject::tr("Critical"); break;
        case QtFatalMsg    : Severity = QObject::tr("Fatal");    break;
        }
    QString LineMsg = QString::asprintf("(%24s:%5u) - %8s - %s - %s\n",
                                        Context.file, 
                                        Context.line, 
                                        C_STRING(Severity),
                                        C_STRING(Message),
                                        Context.function);
    switch (Type) 
        {
        case QtDebugMsg :
        case QtInfoMsg  :
        case QtWarningMsg:
        case QtCriticalMsg:
            fprintf(stderr, C_STRING(LineMsg));
            break;
        case QtFatalMsg:
            fprintf(stderr, C_STRING(LineMsg));
            abort();
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

int main(int argc, char* argv[])
    {
    qInstallMessageHandler(CustomMessageHandler);

    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName("cbAsm8080");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser CLParser;
    QCommandLineOption OGenList(QStringList() << "l" << "list",   "Generate assembly listing.");
    QCommandLineOption OGenAST (QStringList() << "a" << "ast",    "Generate AST listing.");
    QCommandLineOption OGenIds (QStringList() << "i" << "ids",    "Generate identifier listing.");
    QCommandLineOption OStrict (QStringList() << "s" << "strict", "Strict labels and names.");

    QCommandLineOption OOutDir (QStringList() << "o" << "outdir", "Output directory." , "OutDir");

    CLParser.addHelpOption();
    CLParser.addVersionOption();
    CLParser.addPositionalArgument("source", "Assembler source file.");
    CLParser.addOption(OStrict);
    CLParser.addOption(OGenList);
    CLParser.addOption(OGenAST);
    CLParser.addOption(OGenIds);
    CLParser.addOption(OOutDir);

    CLParser.process(app);

    if (CLParser.positionalArguments().isEmpty())
        {
        printf(CLParser.helpText().toLocal8Bit().data());
        exit(EXIT_FAILURE);
        }
    QString InFileName = CLParser.positionalArguments().at(0);
    QString OutDirName = CLParser.value(OOutDir);
    bool    GenList    = CLParser.isSet(OGenList);
    bool    GenAST     = CLParser.isSet(OGenAST);
    bool    GenIds     = CLParser.isSet(OGenIds);
            Strict     = CLParser.isSet(OStrict);

    QString HexFileName  = OutFileName(InFileName, OutDirName, "hex");
    QString BinFileName  = OutFileName(InFileName, OutDirName, "bin");
    QString ListFileName = OutFileName(InFileName, OutDirName, "lst");
    QString ASTFileName  = OutFileName(InFileName, OutDirName, "ast");
    QString IdsFileName  = OutFileName(InFileName, OutDirName, "ids");

    printf("Input file : %s\n", C_STRING(InFileName));
    printf("Hex file   : %s\n", C_STRING(HexFileName));
    printf("Bin file   : %s\n", C_STRING(BinFileName));
    if (GenList) printf("List file  : %s\n", C_STRING(ListFileName));
    if (GenAST)  printf("AST file   : %s\n", C_STRING(ASTFileName));
    if (GenIds)  printf("Ids file   : %s\n", C_STRING(IdsFileName));

    FILE* InFile = fopen(C_STRING(InFileName), "r");
    if (!InFile)
        {
        fprintf(stderr, "Could not open '%s'.\n", C_STRING(InFileName));
        exit(EXIT_FAILURE);
        }

    FILE* HexFile = fopen(C_STRING(HexFileName), "w");
    if (!HexFile)
        {
        fprintf(stderr, "Could not open '%s'.\n", C_STRING(HexFileName));
        exit(EXIT_FAILURE);
        }

    FILE* BinFile = fopen(C_STRING(BinFileName), "wb");
    if (!BinFile)
        {
        fprintf(stderr, "Could not open '%s'.\n", C_STRING(BinFileName));
        exit(EXIT_FAILURE);
        }

    FILE* ListFile;
    if (GenList)
        {
        ListFile = fopen(C_STRING(ListFileName), "w");
        if (!ListFile)
            {
            fprintf(stderr, "Could not open '%s'.\n", C_STRING(ListFileName));
            exit(EXIT_FAILURE);
            }
        }

    FILE* ASTFile;
    if (GenAST)
        {
        ASTFile = fopen(C_STRING(ASTFileName), "w");
        if (!ASTFile)
            {
            fprintf(stderr, "Could not open '%s'.\n", C_STRING(ASTFileName));
            exit(EXIT_FAILURE);
            }
        }

    FILE* IdsFile;
    if (GenIds)
        {
        IdsFile = fopen(C_STRING(IdsFileName), "w");
        if (!IdsFile)
            {
            fprintf(stderr, "Could not open '%s'.\n", C_STRING(IdsFileName));
            exit(EXIT_FAILURE);
            }
        }

    Unit.Process(InFile);
    Unit.DumpHex(HexFile);
    Unit.DumpBin(BinFile);

    if (GenList)
        {
        Unit.DumpList(ListFile);
        }

    if (GenAST)
        {   
        Unit.DumpAST(ASTFile);
        }

    if (GenIds)
        {
        Unit.DumpIdentifiers(IdsFile);
        }

    if (GlobalError)
        {
        fprintf(stderr, "Encountered errors during assembling.\n");
        exit(EXIT_FAILURE);
        }
    else
        {
        fprintf(stderr, "Successfully assembled.\n");
        exit(EXIT_SUCCESS);
        }
    }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// vim: syntax=cpp ts=4 sw=4 sts=4 sr et columns=100 lines=45 

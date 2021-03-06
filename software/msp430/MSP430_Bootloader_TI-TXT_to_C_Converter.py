"""
An MSP430 txt Binary Compiled File to C structure converter.

requires Python 2.7+. 

By: Jeremy Hunt
"""

import argparse, time, re

def parseBinaryTextFile(filename):
    """ Parses an input MSP430 TI-TXT binary file into a list of dictionaries representing
        the binary data. 
    """
    inputText = ""
    with open(filename) as f:
        inputText = f.read()

    inputLines = inputText.split("\n")

    #Read the lines one at a time with mode
    convertedFileInfo = []
    dataSectionNumber = -1
    mode = None
    for line in inputLines:
        line = line.strip()
        if line == "q":
            mode = None
            break
        elif line[0] == "@" and (mode == None or mode == "readData"):
            mode = "readData"
            dataSectionNumber += 1
            convertedFileInfo.append({})
            convertedFileInfo[dataSectionNumber]["address"] = int(line[1:],16)
            continue
        elif mode == "readData":
            #Add the data in the line to an array
            if "data" not in convertedFileInfo[dataSectionNumber]:
                convertedFileInfo[dataSectionNumber]["data"] = []
            convertedFileInfo[dataSectionNumber]["data"].extend(map(lambda num: int(num, 16), line.split(" ")))
        else:
            print "Error Reading File..."

    #Return the file data
    return convertedFileInfo

def createCStructureFile(convertedFileInfo, inputFilename):
    #Create the text for a new C structure file
    cFileHeader = """/*
 *  """+inputFilename+"""
 *  
 *  Auto-Generated On: """ + time.strftime("%b %d, %Y at %I:%M %p") + """
 *  Author: Jeremy Hunt
 */
 
#ifndef MSP430_PROGRAM_DATA_H_
#define MSP430_PROGRAM_DATA_H_
    
/*Header Containing Section Structure*/
struct MSP430_PROGRAM_SECTION {
    const uint16 MSP430_SECTION_ADDRESS;
    const uint16 MSP430_SECTION_SIZE;
    const unsigned char *MSP430_SECTION_DATA;
};

/*Section Data Arrays*/
"""

    convertedCFile = ""
    convertedCFile += cFileHeader
    convertedCFile += "#define MSP430_PROGRAM_NUM_SECTIONS " + str(len(convertedFileInfo)) + "\n"
    sectionCount = 0
    for section in convertedFileInfo:
        convertedCFile += "const unsigned char MSP430_SECTION_DATA_ARRAY_"+str(sectionCount)+\
                            "["+str(len(section["data"]))+"] = {"+\
                            ", ".join(map(lambda byte: ("0x{:02X}".format(byte)), section["data"]))+\
                            "};\n"
        sectionCount += 1

    convertedCFile += """
/*Array and Section Definition*/
const struct MSP430_PROGRAM_SECTION MSP430_PROGRAM[MSP430_PROGRAM_NUM_SECTIONS] = {
"""
    sectionCount = 0
    for section in convertedFileInfo:
        convertedCFile += "    {\n"
        convertedCFile += "        " + ("0x{:04X}".format(section["address"])) + ", /*MSP430_SECTION_ADDRESS*/\n"
        convertedCFile += "        " + str(len(section["data"])) + ", /*MSP430_SECTION_SIZE*/\n"
        convertedCFile += "        " + "MSP430_SECTION_DATA_ARRAY_" + str(sectionCount) + " /*MSP430_SECTION_DATA*/\n"
        convertedCFile += "    }"
        if sectionCount < (len(convertedFileInfo)-1): #Don't Add a comma to the last one
            convertedCFile += ","
        convertedCFile += "\n"
        sectionCount += 1

    convertedCFile += "};\n\n"
    convertedCFile += "#endif /* MSP430_PROGRAM_DATA_H_ */\n"

    #Return it!
    return convertedCFile


#Get and parse the command line arguments
parser = argparse.ArgumentParser(description="Converts MSP430 txt Binary Compiled Files to C structures.")
parser.add_argument("inputFilename", help="The name of the input MSP430 TXT binary file.")
parser.add_argument("outputFilename", help="The name of the converted C structure.")
args = parser.parse_args()

#Read and parse the file
convertedFileInfo = parseBinaryTextFile(args.inputFilename)

convertedCFileText = createCStructureFile(convertedFileInfo, args.inputFilename)
#Write the file back
with open(args.outputFilename, "w") as f:
    f.write(convertedCFileText)

#Finished!
print "Done."

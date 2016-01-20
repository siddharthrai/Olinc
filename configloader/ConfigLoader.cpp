/**************************************************************************
 *
 * Copyright (c) 2002 - 2011 by Computer Architecture Department,
 * Universitat Politecnica de Catalunya.
 * All rights reserved.
 *
 * The contents of this file may not be disclosed to third parties,
 * copied or duplicated in any form, in whole or in part, without the
 * prior permission of the authors, Computer Architecture Department
 * and Universitat Politecnica de Catalunya.
 *
 * $RCSfile: ConfigLoader.cpp,v $
 * $Revision: 1.40 $
 * $Author: cgonzale $
 * $Date: 2008-06-02 17:37:34 $
 *
 * ConfigLoader implementation file.
 *
 */

/**
 *
 *  @file ConfigLoader.cpp
 *
 *  This file implements the ConfigLoader class that is used
 *  to parse and load the simulator configuration.
 *
 *
 */

#include "ConfigLoader.h"
#include <cstring>
#include <cstdio>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdlib>

using namespace std;

/**
 * Use free(line) to deallocate dynamic memory
 *
 * see "man getline"
 */
int ConfigLoader::getline(char **lineptr, size_t *n, FILE *file)
{
    if ( feof(file) )
        return -1;

    vector<char> linev;
    while ( !feof(file) )
    {
        char c = fgetc(file);
        linev.push_back(c);
        if ( c == '\n' )
            break;
    }

    if ( feof(file) )
        return -1;


    // update buffer size
    *n = (size_t)linev.size() + 1;

    if ( *lineptr == NULL )
        *lineptr = (char*)malloc(*n*sizeof(char));
    else
        *lineptr = (char*)realloc(*lineptr, *n*sizeof(char));

    ub4 i; // Copy contents
    for ( i = 0; i < linev.size(); i++ )
        (*lineptr)[i] = linev[i];
    (*lineptr)[i] = char(0);

    return static_cast<int>(linev.size()); // end of line is not included in the count
}

/*  ConfigLoader constructor.  */
ConfigLoader::ConfigLoader(char *filename) :

    Parser(NULL, 0)

{
    /*  Tries to open the config file.  */
    configFile = fopen(filename, "r");

    /*  Check if file was correctly opened.  */
    if (configFile == NULL)
        panic("ConfigLoader", "ConfigLoader", "Error opening configuration file.");
}

/*  Config Loader destructor.  */
ConfigLoader::~ConfigLoader()
{
    /*  Close the config file.  */
    fclose(configFile);
}

/*  Loads, parses and stores the simulator parameters from the
    configuration file.  */
void ConfigLoader::getParameters(SimParameters *simP)
{
    bool end;

    /*  Not yet the end of config file parsing.  */
    end = FALSE;

    /*  Read first line from the config file.  */
    if (getline(&str, &len, configFile) <= 0)
    {
            /*  Check if it is end of file.  */
            if (feof(configFile))
                end = TRUE;
            else
                panic("ConfigLoader", "getParameters", "Error while reading the configuration file.");

    }

    /*  Parse all the simulator sections.  */
    while (!end)
    {
        /*  Set line parse start position to 0.  */
        pos = 0;

        /*  First skip any initial spaces or tabulators.  */
        skipSpaces();

        /*  Second skip all the comment and empty lines.  */
        if ((str[pos] != '#') && (str[pos] != '\n') && !feof(configFile))
        {
            /*  Parse a section.  */
            if (!parseSection(simP))
            {
                panic("ConfigLoader", "getParameters", "Error parsing configuration file section.");
            }
        }
        else
        {
            /*  Read a new line from the config file.  */
            if (getline(&str, &len, configFile) <= 0)
            {
                /*  Check if it is end of file.  */
                if (feof(configFile))
                    end = TRUE;
                else
                    panic("ConfigLoader", "getParameters", "Error while reading the configuration file.");
            }
        }
    }

    paramsTracker.checkParamsIntegrity(simP);
}

/*  Parse a configuration section.  */
bool ConfigLoader::parseSection(SimParameters *simP)
{
    char sectionName[100];
    Section section;

    /*  Check open section name character.  */
    if (str[pos] != '[')
        return FALSE;

    /*  Update position.  */
    pos++;

    /*  Check end of line.  */
    checkEndOfString

    /*  Try to parse section name.  */
    if (!parseId(sectionName))
        return FALSE;

    /*  Check end of the line.  */
    checkEndOfString

    /*  Check end section name character.  */
    if (str[pos] != ']')
        return FALSE;

    /*  Reset section.  */
    section = SEC_UKN;

    /*  Select section.  */

    if (!strcmp("LLC", sectionName))
    {
        section = SEC_LLC;
    }

    /*  Check if it was a known section.  */
    if (section == SEC_UKN)
    {
        return FALSE;
    }
    
    /*  Parse the section parameters.  */
    return parseSectionParameters(section, simP);
}

/*  Parse all the section parameters.  */
bool ConfigLoader::parseSectionParameters(Section section, SimParameters *simP)
{
    bool end;
    ub4 allocSigParam;

    /*  Not end of the section yet.  */
    end = FALSE;

    /*  Parse all the section parameters.  */
    while (!end)
    {
        /*  Set line start position.  */
        pos = 0;

        /*  Read next line.  */
        if (getline(&str, &len, configFile) > 0)
        {
            /*  First skip any initial space or tab.  */
            skipSpaces();

            /*  Second, skip empty lines and comments lines.  */
            if ((str[pos] != '#') && (str[pos] != '\n'))
            {
                /*  third, check is not the start of another section.  */
                if (str[pos] != '[')
                {
                    /*  Check end of line.  */
                    checkEndOfString

                    /*  Select section.  */
                    switch(section)
                    {
                        case SEC_LLC:

                            /*  Try to parse a DAC parameter.  */
                            if (!parseLCSectionParameter(&simP->lcP))
                                return FALSE;
                            break;

                    }
                }
                else
                {
                    /*  End of section.  */
                    end = TRUE;
                }
            }
        }
        else
        {
            /*  End of file or error.  */

            /*  Check end of file.  */
            if (feof(configFile))
            {
                end = TRUE;
            }
            else
                panic("ConfigLoader", "parseSimSection", "Error while reading config file.");
        }
    }

    return TRUE;
}

/*  Parse LLC parameters.  */
bool ConfigLoader::parseLCSectionParameter(LChParameters *lcP)
{
    char id[100];

    /*  Try to get a parameter name.  */
    if (!parseId(id))
        return FALSE;

    checkEndOfString

    skipSpaces();

    checkEndOfString

    /*  Check '='.  */
    if (str[pos] != '=')
        return FALSE;

    /*  Update position.  */
    pos++;

    checkEndOfString

    skipSpaces();

    checkEndOfString

    paramsTracker.startParamSectionDefinition("LLC");

    /*  Parse parameters.  */

    if (!parseDecimalParameter("MinCacheSize", id, lcP->minCacheSize))
        return FALSE;

    if (!parseDecimalParameter("MaxCacheSize", id, lcP->maxCacheSize))
        return FALSE;

    if (!parseDecimalParameter("ClockPeriod", id, lcP->clockPeriod))
        return FALSE;

    if (!parseDecimalParameter("MinCacheWays", id, lcP->minCacheWays))
        return FALSE;

    if (!parseDecimalParameter("MaxCacheWays", id, lcP->maxCacheWays))
        return FALSE;

    if (!parseDecimalParameter("SamplerSets", id, lcP->samplerSets))
        return FALSE;

    if (!parseDecimalParameter("SamplerWays", id, lcP->samplerWays))
        return FALSE;

    if (!parseDecimalParameter("CacheBlockSize", id, lcP->cacheBlockSize))
        return FALSE;

    if (!parseDecimalParameter("MaxRRPV", id, lcP->max_rrpv))
        return FALSE;

    if (!parseDecimalParameter("SpillRRPV", id, lcP->spill_rrpv))
        return FALSE;

    if (!parseDecimalParameter("Threshold", id, lcP->threshold))
        return FALSE;

    if (!parseDecimalParameter("Streams", id, lcP->streams))
        return FALSE;

    if (!parseBooleanParameter("GPUEnable", id, lcP->gpu_enable))
        return FALSE;

    if (!parseBooleanParameter("CPUEnable", id, lcP->cpu_enable))
        return FALSE;

    if (!parseBooleanParameter("GPGPUEnable", id, lcP->gpgpu_enable))
        return FALSE;

    if (!parseCachePolicy("Policy", id, lcP->policy))
        return FALSE;

    if (!parseStream("Stream", id, lcP->stream))
        return FALSE;

    if (!parseStringParameter("StatFile", id, lcP->statFile))
        return FALSE;

    if (!parseBooleanParameter("UseVa", id, lcP->useVa))
        return FALSE;

    if (!parseBooleanParameter("UseBS", id, lcP->useBs))
        return FALSE;

    if (!parseBooleanParameter("UseStep", id, lcP->useStep))
        return FALSE;

    if (!parseBooleanParameter("PinBlocks", id, lcP->pinBlocks))
        return FALSE;

    if (!parseBooleanParameter("SpeedupEnabled", id, lcP->speedupEnabled))
        return FALSE;

    if (!parseBooleanParameter("RemapCritical", id, lcP->remapCrtcl))
        return FALSE;

    if (!parseDecimalParameter("ShctSize", id, lcP->shctSize))
        return FALSE;

    if (!parseDecimalParameter("SignSize", id, lcP->signSize))
        return FALSE;

    if (!parseDecimalParameter("CoreSize", id, lcP->coreSize))
        return FALSE;

    if (!parseDecimalParameter("EntrySize", id, lcP->entrySize))
        return FALSE;

    if (!parseBooleanParameter("UseMem", id, lcP->useMem))
        return FALSE;

    if (!parseBooleanParameter("UsePc", id, lcP->usePc))
        return FALSE;

    if (!parseBooleanParameter("CpuFillEnable", id, lcP->cpuFillEnable))
        return FALSE;

    if (!parseBooleanParameter("GpuFillEnable", id, lcP->gpuFillEnable))
        return FALSE;

    if (!parseBooleanParameter("DRAMSimEnable", id, lcP->dramSimEnable))
        return FALSE;

    if (!parseBooleanParameter("DRAMSimTrace", id, lcP->dramSimTrace))
        return FALSE;

    if (!parseStringParameter("DRAMConfigFile", id, lcP->dramSimConfigFile))
        return FALSE;

    if (!parseStringParameter("PriorityStream", id, lcP->dramPriorityStream))
        return FALSE;

    if ( !paramsTracker.wasAnyParamSectionDefined() ) {
        stringstream ss;
        ss << "Parameter '" << id << "' in section [LLC] is not supported";
        panic("ConfigLoader", "parseLCSectionParameter", ss.str().c_str());
    }

    return TRUE;
}

/*  Parses a decimal parameter (32 bit version)  */
bool ConfigLoader::parseDecimalParameter(char *paramName, char *id, ub4 &val)
{
    sb4 aux;

    paramsTracker.registerParam(paramName);
    //registerParam(_sectionStr, paramName);

    /*  Search section parameter name.  */
    if (!strcmp(paramName, id))
    {
        /*  Try to parse a number.  */
        if (!parseDecimal(aux))
            return FALSE;

        /*  Set decimal parameter.  */
        val = aux;
        paramsTracker.setParamDefined(id);
    }

    return TRUE;
}

/*  Parses a decimal parameter (64 bit version).  */
bool ConfigLoader::parseDecimalParameter(char *paramName, char *id, ub8 &val)
{
    sb8 aux;

    paramsTracker.registerParam(paramName);

    /*  Search section parameter name.  */
    if (!strcmp(paramName, id))
    {
        /*  Try to parse a number.  */
        if (!parseDecimal(aux))
            return FALSE;

        /*  Set decimal parameter.  */
        val = aux;
        paramsTracker.setParamDefined(id);
    }

    return TRUE;
}

/*  Parses a float parameter (32 bit).  */
bool ConfigLoader::parseFloatParameter(char *paramName, char *id, uf4 &val)
{
    uf4 aux;

    paramsTracker.registerParam(paramName);

    /*  Search section parameter name.  */
    if (!strcmp(paramName, id))
    {
        /*  Try to parse a number.  */
        if (!parseFP(aux))
            return FALSE;

        /*  Set float parameter.  */
        val = aux;
        paramsTracker.setParamDefined(id);
    }

    return TRUE;
}

/*  Parse a string parameter.  */
bool ConfigLoader::parseStringParameter(char *paramName, char *id,
    char *&string)
{
    char *auxStr;

    paramsTracker.registerParam(paramName);

    if (!strcmp(paramName, id))
    {
        /*  Try to parse a string.  */
        if (!parseString(auxStr))
            return FALSE;

        /*  Return string parameter.  */
        string = auxStr;
        paramsTracker.setParamDefined(id);
    }

    return TRUE;
}

/*  Parse a boolean parameter.  */
bool ConfigLoader::parseBooleanParameter(char *paramName, char *id, bool &val)
{
    bool aux;

    paramsTracker.registerParam(paramName);

    if (!strcmp(paramName, id))
    {
        /*  Try to parse a string.  */
        if (!parseBoolean(aux))
            return FALSE;

        /*  Return boolean parameter.  */
        val = aux;
        paramsTracker.setParamDefined(id);
    }

    return TRUE;
}

/*  Parse cache policy parameter.  */
bool ConfigLoader::parseCachePolicy(char *paramName, char *id, enum cache_policy_t &val)
{
    char *auxStr;

    paramsTracker.registerParam(paramName);

    if (!strcmp(paramName, id))
    {
        /*  Try to parse a string.  */
        if (!parseString(auxStr))
            return FALSE;

        /*  Set optimal policy  */
        if (!strcmp(auxStr, "OPT"))
        {
          val = cache_policy_opt;

          goto end;
        }

        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "NRU"))
        {
          val = cache_policy_nru;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "UCP"))
        {
          val = cache_policy_ucp;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "SALRU"))
        {
          val = cache_policy_salru;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "LRU"))
        {
          val = cache_policy_lru;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "FIFO"))
        {
          val = cache_policy_fifo;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "LIP"))
        {
          val = cache_policy_lip;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "BIP"))
        {
          val = cache_policy_bip;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "DIP"))
        {
          val = cache_policy_dip;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "SRRIP"))
        {
          val = cache_policy_srrip;

          goto end;
        }

        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "SRRIPBYPASS"))
        {
          val = cache_policy_srripbypass;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "SRRIPHINT"))
        {
          val = cache_policy_srriphint;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "SRRIPBS"))
        {
          val = cache_policy_srripbs;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "SRRIPSAGE"))
        {
          val = cache_policy_srripsage;

          goto end;
        }

        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "SRRIPDBP"))
        {
          val = cache_policy_srripdbp;

          goto end;
        }

        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "XSPDBP"))
        {
          val = cache_policy_xspdbp;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "PASRRIP"))
        {
          val = cache_policy_pasrrip;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "XSP"))
        {
          val = cache_policy_xsp;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "XSPPIN"))
        {
          val = cache_policy_xsppin;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "STREAMPIN"))
        {
          val = cache_policy_streampin;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "BRRIP"))
        {
          val = cache_policy_brrip;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "DRRIP"))
        {
          val = cache_policy_drrip;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "GSDRRIP"))
        {
          val = cache_policy_gsdrrip;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "GSPC"))
        {
          val = cache_policy_gspc;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "GSPCM"))
        {
          val = cache_policy_gspcm;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "GSPCT"))
        {
          val = cache_policy_gspct;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "GSHP"))
        {
          val = cache_policy_gshp;

          goto end;
        }

        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "SAPSIMPLE"))
        {
          val = cache_policy_sapsimple;

          goto end;
        }
       
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "SAPDBP"))
        {
          val = cache_policy_sapdbp;

          goto end;
        }

        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "SAPPRIORITY"))
        {
          val = cache_policy_sappriority;

          goto end;
        }
       
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "SAPPRIDEPRI"))
        {
          val = cache_policy_sappridepri;

          goto end;
        }
       
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "SARP"))
        {
          val = cache_policy_sarp;

          goto end;
        }
       
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "SHIP"))
        {
          val = cache_policy_ship;

          goto end;
        }
       
        panic("Unknown policy\n");
        exit(-1);
end:
        paramsTracker.setParamDefined(id);
    }
    
    return TRUE;
}

/*  Parse cache policy parameter.  */
bool ConfigLoader::parseStream(char *paramName, char *id, ub1 &val)
{
    char *auxStr;

    paramsTracker.registerParam(paramName);

    if (!strcmp(paramName, id))
    {
        /*  Try to parse a string.  */
        if (!parseString(auxStr))
            return FALSE;

        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "IS"))
        {
          val = IS;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "CS"))
        {
          val = CS;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "ZS"))
        {
          val = ZS;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "TS"))
        {
          val = TS;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "NS"))
        {
          val = NS;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "HS"))
        {
          val = HS;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "BS"))
        {
          val = BS;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "DS"))
        {
          val = DS;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "XS"))
        {
          val = XS;

          goto end;
        }
        
        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "PS"))
        {
          val = PS;

          goto end;
        }

        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "OS"))
        {
          val = OS;

          goto end;
        }

        /*  Return boolean parameter.  */
        if (!strcmp(auxStr, "ALL"))
        {
          val = NN;

          goto end;
        }
       
        panic("Unknown stream\n");
        exit(-1);
end:
        paramsTracker.setParamDefined(id);
    }
    
    return TRUE;
}

//////////////////////////////////////////////////////////////////////////////////////
//                              ParamsTracker methods                               //
//////////////////////////////////////////////////////////////////////////////////////

void ConfigLoader::ParamsTracker::startParamSectionDefinition(const string& sec)
{
    paramSectionDefinedFlag = false; 
    secStr = sec;
}

bool ConfigLoader::ParamsTracker::wasAnyParamSectionDefined() const
{
    return paramSectionDefinedFlag;
}

void ConfigLoader::ParamsTracker::setParamDefined(const std::string& paramName)
{
    map<string,bool>::iterator it = paramsSeen.find(secStr + "=" + paramName);
    if ( it == paramsSeen.end() ) {
        stringstream ss;
        ss << "Param [" << secStr << "] -> " << paramName << " was not previously registered before define it";
        panic("ConfigLoader", "ParamsTracker::setParamDefined", ss.str().c_str());
    }
    it->second =  true;
    paramSectionDefinedFlag = true;
}

void ConfigLoader::ParamsTracker::registerParam(const std::string& paramName)
{
    map<string,bool>::iterator it = paramsSeen.find(secStr + "=" + paramName);
    if ( it == paramsSeen.end() )
        paramsSeen.insert(make_pair(secStr + "=" + paramName, false)); // Add this param to the map
}

/*
bool ConfigLoader::ParamsTracker::isParamDefined(const string& sec, const string& paramName) const
{
    map<string,bool>::const_iterator it = paramsSeen.find(sec + "=" + paramName);
    return (it != paramsSeen.end() && it->second);
}
*/

void ConfigLoader::ParamsTracker::checkParamsIntegrity(SimParameters *simP)
{   
    // Traverse params seen, all params should be true
    map<string,bool>::const_iterator it = paramsSeen.begin();
    for ( ; it != paramsSeen.end(); ++it ) {
        if ( !it->second ) {
            stringstream ss;
            size_t pos = it->first.find_first_of("=");
            string secName = it->first.substr(0,pos);
            string pName   = it->first.substr(pos+1);
            // ss << "Param '" << pName << "' from section '" << secName << "' expected and wasn't found";
            ss << "Param [" << secName << "] : '" << pName << "' expected and wasn't found";
            panic("ConfigLoader", "ParamsTracker::checkParamsIntegrity", ss.str().c_str());
        }
    }
}


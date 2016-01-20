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
 * $RCSfile: ConfigLoader.h,v $
 * $Revision: 1.38 $
 * $Author: cgonzale $
 * $Date: 2008-06-02 17:37:34 $
 *
 * ConfigLoader Definition file.
 *
 */

/**
 *
 *  @file ConfigLoader.h
 *
 *  This file defines the ConfigLoader class that loads and parses
 *  the simulator configuration file.
 *
 *
 */

#ifndef _CONFIGLOADER_

#define _CONFIGLOADER_

/*
    Needed for using getline (GNU extension).
    Compatibility problems??

    Must be the first include because Parser is also including it.
*/

#include <cstdio>
#include <map>
#include <string>
#include "common/intermod-common.h"
#include "policy.h"
#include "Parser.h"

/***  The initial size of the buffer for the Signal section.  */
static const ub4 SIGNAL_BUFFER_SIZE = 200;

/**
 *
 *  Defines the configuration file sections.
 *
 */

enum Section
{
    SEC_LLC,    /* LLC section */
    SEC_UKN     /*  Unknown section.  */
};

/*
 *
 * LLC parameters 
 *
 */

struct LChParameters
{
  ub8   minCacheSize;           /* Minimum cache size in KB */
  ub8   maxCacheSize;           /* Maximum cache size in KB */
  ub4   clockPeriod;            /* Clock frequency */
  ub4   cacheBlockSize;         /* Block size in bytes */
  ub4   minCacheWays;           /* Ways per set, 0 for fully associative */
  ub4   maxCacheWays;           /* Ways per set, 0 for fully associative */
  ub4   max_rrpv;               /* Ways per set, 0 for fully associative */
  ub4   spill_rrpv;             /* RRPV assigned to spills */
  ub4   threshold;              /* Threshold for probabilistic policy */
  ub4   samplerSets;            /* Sampler sets in sampler based policy */
  ub4   samplerWays;            /* Sampler ways in sampler based policy */
  ub4   streams;                /* Total number of streams to be simulated */
  bool  cpu_enable;             /* True, if CPU is enable */
  bool  gpu_enable;             /* True, if GPU is enable */
  bool  gpgpu_enable;           /* True, if GPGPU is enable */
  sb1  *statFile;               /* Statistics collection file */
  ub1   stream;                 /* Stream to be simulated */
  bool  useVa;                  /* True, if va is to be used */
  bool  useBs;                  /* True, if baseline samples are used */
  bool  speedupEnabled;         /* True, if speedup hints are used */
  bool  remapCrtcl;             /* True, if pages are remapped to critical rows */
  bool  useStep;                /* True, if step function is to be used in sappridepri policy */
  bool  pinBlocks;              /* True, SARP blocks are pinned */
  ub4   shctSize;               /* Ship counter table size */
  ub4   signSize;               /* Ship sign size */
  ub4   coreSize;               /* Log of core count */
  ub4   entrySize;              /* Ship signature entry size */
  bool  useMem;                 /* If true, shipmem is used */
  bool  usePc;                  /* If true, shippc is used */
  bool  cpuFillEnable;          /* If true, CPU fill is enable in SARP */
  bool  gpuFillEnable;          /* If true, GPU fill is enable in SARP */
  bool  dramSimEnable;          /* If true, DRAMSim is enable */
  bool  dramSimTrace;           /* If true, DRAMSim is enable */
  sb1  *dramSimConfigFile;      /* DRAMSim config file */
  sb1  *dramPriorityStream;     /* DRAMSim priority stream */
  enum  cache_policy_t policy;  /* Cache policy */
};

struct SimParameters
{
    LChParameters lcP;          /**<  LLC Parameters */
};

/**
 *
 *  Loads and parses the simulator configuration file.
 *
 *  Inherits from the Parser class that offers basic parsing
 *  functions.
 *
 */

class ConfigLoader : public Parser
{

private:

    /**
     * Helper class designed to add support for checking missing parameters and parameters not supported by ConfigLoader
     */
    class ParamsTracker
    {
    private:

        std::map<std::string,bool> paramsSeen;
        std::string secStr;
        bool paramSectionDefinedFlag;

    public:

        void startParamSectionDefinition(const std::string& sec);

        bool wasAnyParamSectionDefined() const;

        void registerParam(const std::string& paramName);

        void setParamDefined(const std::string& paramName);

        // bool isParamDefined(const std::string& secName, const std::string& paramName) const;

        void checkParamsIntegrity(SimParameters *simP);

    }; // class ParamsTracker

    ParamsTracker paramsTracker;


    FILE *configFile;       /**<  File descriptor for the configuration file.  */

    /*  Private functions.  */

    /**
     *
     *  Parses a section of the configuration file.
     *
     *  @param simP Pointer to the Simulator Parameters structure where to
     *  store the section parameters.
     *
     *  @return TRUE if the section was succefully parsed.
     *
     */

    bool parseSection(SimParameters *simP);

    /**
     *
     *  Parses all the section parameters for a given section.
     *
     *  @param section The section that is going to be parsed.
     *  @param simP Pointer to the simulator parameters structure where
     *  to store the parsed parameters.
     *
     *  @return TRUE if the section parameters were parsed correctly.
     *
     */

    bool parseSectionParameters(Section section, SimParameters *simP);

    /**
     *
     *  Parses a LLC section parameter.
     *
     *  @param lcP Pointer to the LLC Parameters structure
     *  where to store the parsed parameter.
     *
     *  @return TRUE if the parameter was parsed correctly.
     *
     */

    bool parseLCSectionParameter(LChParameters *lcP);


    /**
     *
     *  Parses a decimal parameter.
     *
     *  @param paramName Name of the parameter being parsed.
     *  @param id Name of the parameter to parse.
     *  @param val Reference to a 32bit integer.
     *
     *  @return TRUE if the parameter was parsed.
     *
     */

    bool parseDecimalParameter(char *paramName, char *id, ub4 &val);

    /**
     *
     *  Parses a decimal parameter.
     *
     *  @param paramName Name of the parameter being parsed.
     *  @param id Name of the parameter to parse.
     *  @param val Reference to a 64 bit integer.
     *
     *  @return TRUE if the parameter was parsed.
     *
     */

    bool parseDecimalParameter(char *paramName, char *id, ub8 &val);

    /**
     *
     *  Parses a 32-bit floating point parameter.
     *
     *  @param paramName Name of the parameter being parsed.
     *  @param id Name of the parameter to parse.
     *  @param val Reference to a 32 bit floating point.
     *
     *  @return TRUE if the parameter was parsed.
     *
     */

    bool parseFloatParameter(char *paramName, char *id, uf4 &val);

    /**
     *
     *  Parses a boolean parameter.
     *
     *  @param paramName Name of the parameter being parsed.
     *  @param id Name of the parameter to parse.
     *  @param val Reference to a boolean variable were to write the value of the parameter.
     *
     *  @return TRUE if the parameter was parsed.
     *
     */

    bool parseBooleanParameter(char *paramName, char *id, bool &val);

    /**
     *
     *  Parses a string parameter.
     *
     *  @param paramName Name of the parameter being parsed.
     *  @param id Name of the parameter to parse.
     *  @param string Reference to a string pointer.
     *
     *  @param return TRUE if the parameter was correctly parsed.
     *
     */

    bool parseStringParameter(char *paramName, char *id, char *&string);

    /**
     *
     *  Parses a cache policy.
     *
     *  @param paramName Name of the parameter being parsed.
     *  @param id Name of the parameter to parse.
     *  @param policy Reference to a policy pointer.
     *
     *  @param return TRUE if the parameter was correctly parsed.
     *
     */
    bool parseCachePolicy(char *paramName, char *id, enum cache_policy_t &policy);

    /**
     *
     *  Parses a access stream to be simulated.
     *
     *  @param paramName Name of the parameter being parsed.
     *  @param id Name of the parameter to parse.
     *  @param stream Reference to a stream pointer.
     *
     *  @param return TRUE if the parameter was correctly parsed.
     *
     */
    bool parseStream(char *paramName, char *id, ub1 &stream);

    int getline(char **lineptr, size_t *n, FILE *stream);

public:

    /**
     *
     *  Config Loader constructor.
     *
     *  Creates and initializes a ConfigLoader object.
     *
     *  Tries to open the configuration file and initializes
     *  the object.
     *
     *  @param configFile The config file name.
     *
     *  @return A new initialized Config Loader object.
     *
     */

    ConfigLoader(char *configFile);

    /**
     *
     *  ConfigLoader destructor.
     *
     *  Deletes a ConfigLoader object.
     *
     *  Closes the configuration file and deletes all the ConfigLoader
     *  object structures.
     *
     */

    ~ConfigLoader();

    /**
     *
     *  Parses the config file and filles the simulator
     *  parameter structure.
     *
     *  @param simParam Pointer to the Simulator Parameters
     *  structure where the parsed parameters are going to
     *  be stored.
     *
     */

    void getParameters(SimParameters *simParam);

};

#endif

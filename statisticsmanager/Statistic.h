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
 */

#ifndef STATISTIC_H
    #define STATISTIC_H

#include <string>
#include <iostream>
#include "../common/intermod-common.h"
static const int MAX_FREQS = 3;

class Statistic
{
private:

    std::string name;
    std::string owner;

protected:

    static bool disabled;   /* Global statistic switch */
    bool statisticDisabled; /* Per statistic switch */
    int freq;

public:

    static void enable();
    static void disable();
    static bool isEnabled();
    void disableStatistic();
    void enableStatistic();
    bool isStatisticEnabled();

    /* must be called in subclass constructor */
    Statistic(std::string name);

    /* must be implemented in subclass */
    virtual Statistic& inc(int times=1)=0;

    /* must be implemented in subclass */
    virtual Statistic& clear(int f=0)=0;

    /*
     * Must be implemented in subclass, it should return a string
     * identifying the subclass
     */
    virtual std::string getString() const=0;

    virtual bool isZero(int f=0) const=0;

    Statistic& operator++();
    Statistic& operator++(int);
    Statistic& operator--();
    Statistic& operator--(int);

    virtual Statistic& incavg(int times=1)=0;

    std::string getName() const;

    void setName(std::string str);

    std::string getOwner() const;

    void setOwner(std::string owner);

    void setCurrentFreq(int i);

    /* redefine this method to get another printing style in derived classes */
    virtual void print(std::ostream& os) const=0;

    friend std::ostream&  operator<<(std::ostream& os, const Statistic& stat)
    {
        stat.print(os);
        return os;
    }

    virtual ~Statistic() = 0;


};




template<class T>
class NumericStatistic : public Statistic
{
protected:

    T value[MAX_FREQS];
    ub8 count[MAX_FREQS];
    bool firstValue[MAX_FREQS];

public:

    using Statistic::disableStatistic;
    using Statistic::enableStatistic;
    using Statistic::isStatisticEnabled;

    NumericStatistic(std::string name) : Statistic(name)
    {
        for(int i = 0; i < MAX_FREQS; i++)
        {
            value[i] = (T)0;
            count[i] = 0;
            firstValue[i] = false;
        }
    }

    NumericStatistic(std::string name, T initialValue) : Statistic(name)
    {
        for(int i = 0; i < MAX_FREQS; i++)
        {
            value[i] = initialValue;
            count[i] = 0;
            firstValue[i] = false;
        }
    }


    std::string getString() const
    {
        return std::string("NUMERIC_STATISTIC");
    }


    NumericStatistic& max(T val)
    {
        for(int i = 0; i < MAX_FREQS; i++)
        {
            value[i] = (!firstValue || (val > value[i]))?val:value[i];
            firstValue[i] = true;
        }

        return *this;
    }

    NumericStatistic& min(T val)
    {
        for(int i = 0; i < MAX_FREQS; i++)
        {
            value[i] = (!firstValue || (val < value[i]))?val:value[i];
            firstValue[i] = true;
        }

        return *this;
    }

    virtual Statistic& clear(int f)
    {
        value[f] = (T)0;
        count[f] = 0;
        firstValue[f] = false;

        return *this;
    }

    virtual Statistic& inc(int times = 1)
    {
        if ( disabled )
            return *this;

        for(int i = 0; i < MAX_FREQS; i++)
        {
            value[i] += times;
        }

        return *this;
    }
    
    void setValue(T val)
    {
      for(int i = 0; i < MAX_FREQS; i++)
      {
        value[i] = val;
      }
    }
    
    T getValue(int freq = 0)
    {
      return value[freq]; 
    }

    virtual Statistic& incavg(int times = 1)
    {
        if ( disabled )
            return *this;
        
        for(int i = 0; i < MAX_FREQS; i++)
        {
            value[i] += times;
            count[i]++;
        }

        return *this;
    }

    virtual void print(std::ostream& os) const
    {
        if (count[freq] == 0)
            os << value[freq];
        else
            os << ((uf4) value[freq])/((uf4) count[freq]);
    }

    virtual bool isZero(int f) const { return (value[f] == (T)0); }


};


#endif // STATISTIC_H

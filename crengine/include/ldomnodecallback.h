/**
 * CoolReader Engine
 *
 * (c) Vadim Lopatin, 2000-2011
 * (c) Other CoolReader authors (See AUTHORS file)
 *
 * This source code is distributed under the terms of
 * GNU General Public License version 2
 * See LICENSE file for details
 */

#ifndef __LDOMNODECALLBACK_H_INCLUDED__
#define __LDOMNODECALLBACK_H_INCLUDED__

class ldomXRange;
class ldomXPointerEx;

/// callback for DOM tree iteration interface
class ldomNodeCallback
{
public:
    /// destructor
    virtual ~ldomNodeCallback() { }
    /// called for each found text fragment in range
    virtual void onText(ldomXRange*) = 0;
    /// called for each found node in range
    virtual bool onElement(ldomXPointerEx*) = 0;
};

#endif // __LDOMNODECALLBACK_H_INCLUDED__

/** @file lvcontaineriteminfo.h

    CoolReader Engine

    (c) Vadim Lopatin, 2000-2006
    (c) Aleksey Chernov, 2022

    This source code is distributed under the terms of
    GNU General Public License.
    See LICENSE file for details.

 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
*/

#ifndef __LVCONTAINERITEMINFO_H_INCLUDED__
#define __LVCONTAINERITEMINFO_H_INCLUDED__

#include <lvtypes.h>

/**
 * @brief The LVContainerItemInfo class
 *
 * Abstract storage class of container element information.
 */
class LVContainerItemInfo
{
public:
    /**
     * @brief Get item size
     * @return item size
     *
     * For an item in an archive container, this is the unpacked size.
     */
    virtual lvsize_t GetSize() const = 0;
    /**
     * @brief Get compressed item size if applicable (for archives).
     * @return compressed item size
     *
     * For an item in an archive container, this is the packed size; for other types of containers, it is undefined.
     */
    virtual lvsize_t GetPackSize() const = 0;
    /**
     * @brief Get item name
     * @return item name
     */
    virtual const lChar32* GetName() const = 0;
    /**
     * @brief Get item's flags.
     * @return Optional item flags. See implementation for details.
     */
    virtual lUInt32 GetFlags() const = 0;
    /**
     * @brief Get the status of an element's container.
     * @return true if this element is a child container; otherwise, false.
     */
    virtual bool IsContainer() const = 0;
    LVContainerItemInfo() { }
    virtual ~LVContainerItemInfo() { }
};

#endif // __LVCONTAINERITEMINFO_H_INCLUDED__

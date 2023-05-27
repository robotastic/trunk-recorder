/* -*- c++ -*- */
/*
 * Copyright 2019 Derek Kozel.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_LATENCY_MANAGER_TAG_TO_MSG_H
#define INCLUDED_LATENCY_MANAGER_TAG_TO_MSG_H

#include "./api.h"
#include <gnuradio/sync_block.h>

namespace gr {
  namespace latency_manager {

    /*!
     * \brief <+description of block+>
     * \ingroup latency_manager
     *
     */
    class LATENCY_MANAGER_API tag_to_msg : virtual public gr::sync_block
    {
     public:
      typedef std::shared_ptr<tag_to_msg> sptr;

      static sptr make(size_t sizeof_stream_item,
                     const std::string& name,
                     const std::string& key_filter = "");

    /*!
     * \brief Returns a vector of tag_t items as of the last call to
     * work.
     */
    virtual std::vector<tag_t> current_tags() = 0;

    /*!
     * \brief Return the total number of tags in the tag queue.
     */
    virtual int num_tags() = 0;

    /*!
     * \brief Set the display of tags to stdout on/off.
     */
    virtual void set_display(bool d) = 0;

    /*!
     * \brief Set a new key to filter with.
     */
    virtual void set_key_filter(const std::string& key_filter) = 0;

    /*!
     * \brief Get the current filter key.
     */
    virtual std::string key_filter() const = 0;
    };
  } // namespace latency_manager
} // namespace gr

#endif /* INCLUDED_LATENCY_MANAGER_TAG_TO_MSG_H */


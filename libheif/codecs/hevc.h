/*
 * HEIF codec.
 * Copyright (c) 2017 Dirk Farin <dirk.farin@gmail.com>
 *
 * This file is part of libheif.
 *
 * libheif is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * libheif is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with libheif.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef HEIF_HEVC_H
#define HEIF_HEVC_H

#include "libheif/heif.h"
#include "box.h"
#include "error.h"

#include <memory>
#include <string>
#include <vector>
#include <codecs/image_item.h>


class Box_hvcC : public Box
{
public:
  Box_hvcC()
  {
    set_short_type(fourcc("hvcC"));
  }

  bool is_essential() const override { return true; }

  struct configuration
  {
    uint8_t configuration_version;
    uint8_t general_profile_space;
    bool general_tier_flag;
    uint8_t general_profile_idc;
    uint32_t general_profile_compatibility_flags;

    static const int NUM_CONSTRAINT_INDICATOR_FLAGS = 48;
    std::bitset<NUM_CONSTRAINT_INDICATOR_FLAGS> general_constraint_indicator_flags;

    uint8_t general_level_idc;

    uint16_t min_spatial_segmentation_idc;
    uint8_t parallelism_type;
    uint8_t chroma_format;
    uint8_t bit_depth_luma;
    uint8_t bit_depth_chroma;
    uint16_t avg_frame_rate;

    uint8_t constant_frame_rate;
    uint8_t num_temporal_layers;
    uint8_t temporal_id_nested;
  };


  std::string dump(Indent&) const override;

  bool get_headers(std::vector<uint8_t>* dest) const;

  void set_configuration(const configuration& config) { m_configuration = config; }

  const configuration& get_configuration() const { return m_configuration; }

  void append_nal_data(const std::vector<uint8_t>& nal);

  void append_nal_data(const uint8_t* data, size_t size);

  Error write(StreamWriter& writer) const override;

protected:
  Error parse(BitstreamRange& range) override;

private:
  struct NalArray
  {
    uint8_t m_array_completeness;
    uint8_t m_NAL_unit_type;

    std::vector<std::vector<uint8_t> > m_nal_units;
  };

  configuration m_configuration;
  uint8_t m_length_size = 4; // default: 4 bytes for NAL unit lengths

  std::vector<NalArray> m_nal_array;
};

class SEIMessage
{
public:
  virtual ~SEIMessage() = default;
};


class SEIMessage_depth_representation_info : public SEIMessage,
                                             public heif_depth_representation_info
{
public:
};


Error decode_hevc_aux_sei_messages(const std::vector<uint8_t>& data,
                                   std::vector<std::shared_ptr<SEIMessage>>& msgs);


Error parse_sps_for_hvcC_configuration(const uint8_t* sps, size_t size,
                                       Box_hvcC::configuration* inout_config,
                                       int* width, int* height);

class ImageItem_HEVC : public ImageItem
{
public:
  ImageItem_HEVC(HeifContext* ctx, heif_item_id id) : ImageItem(ctx, id) {}

  ImageItem_HEVC(HeifContext* ctx) : ImageItem(ctx) {}

  const char* get_infe_type() const override { return "hvc1"; }

  // TODO: MIAF says that the *:hevc:* urn is deprecated and we should use "urn:mpeg:mpegB:cicp:systems:auxiliary:alpha"
  const char* get_auxC_alpha_channel_type() const override { return "urn:mpeg:hevc:2015:auxid:1"; }

  const heif_color_profile_nclx* get_forced_output_nclx() const override { return nullptr; }

  heif_compression_format get_compression_format() const override { return heif_compression_HEVC; }

  Error on_load_file() override;

  int get_luma_bits_per_pixel() const override;

  int get_chroma_bits_per_pixel() const override;

  Result<CodedImageData> encode(const std::shared_ptr<HeifPixelImage>& image,
                                struct heif_encoder* encoder,
                                const struct heif_encoding_options& options,
                                enum heif_image_input_class input_class) override;

protected:
  Result<std::vector<uint8_t>> read_bitstream_configuration_data(heif_item_id itemId) const override;

  std::shared_ptr<class Decoder> get_decoder() const override;

private:
  std::shared_ptr<class Decoder_HEVC> m_decoder;
};

#endif

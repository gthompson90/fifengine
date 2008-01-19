/***************************************************************************
 *   Copyright (C) 2005-2007 by the FIFE Team                              *
 *   fife-public@lists.sourceforge.net                                     *
 *   This file is part of FIFE.                                            *
 *                                                                         *
 *   FIFE is free software; you can redistribute it and/or modify          *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA              *
 ***************************************************************************/

// Standard C++ library includes

// 3rd party library includes

// FIFE includes
// These includes are split up in two parts, separated by one empty line
// First block: files included from the FIFE root src directory
// Second block: files included from the same folder
#include "video/renderbackend.h"
#include "video/image.h"
#include "util/logger.h"

#include "util/fife_math.h"
#include "util/logger.h"
#include "model/metamodel/grids/cellgrid.h"
#include "model/structures/elevation.h"
#include "model/structures/instance.h"
#include "model/structures/layer.h"
#include "model/structures/location.h"

#include "view/camera.h"
#include "camerazonerenderer.h"



namespace FIFE {
	static Logger _log(LM_VIEWVIEW);

	CameraZoneRenderer::CameraZoneRenderer(RenderBackend* renderbackend, ImagePool* imagepool,int position):
		m_renderbackend(renderbackend),
		m_imagepool(imagepool),
		m_zone_image(NULL) {
		setPipelinePosition(position);
		setEnabled(false);
	}

	CameraZoneRenderer::~CameraZoneRenderer() {
		delete m_zone_image;
	}



	void CameraZoneRenderer::render(Camera* cam, Layer* layer, std::vector<Instance*>& instances) {
		CellGrid* cg = layer->getCellGrid();
		if (!cg) {
			FL_WARN(_log, "No cellgrid assigned to layer, cannot draw camera zones");
			return;
		}
		// draw this layer's grid as the camera sees it
		Rect rect = cam->getViewPort();
		if (!m_zone_image) {
			// build zone image
			int dataSize = rect.w * rect.h;

			// initally all pixels are transparent
			uint32_t* data = new uint32_t[dataSize];
			uint32_t* end = data + dataSize;

			#if SDL_BYTEORDER == SDL_BIG_ENDIAN
				uint32_t empty_pixel = 0;//0xff << 24;
				uint32_t edge_pixel = 0xff << 16 | 0xff << 24;
			#else
				uint32_t empty_pixel = 0;//0xff;
				uint32_t edge_pixel = 0xff << 8 | 0xff;
			#endif

			std::fill(data, end, empty_pixel);

			// go from screen coords to grid coords (layer coords)
			// Search through pixel space, drawing pixels at boundaries
			ModelCoordinate prevLayerCoord;
			for (int y = 0; y < rect.h; y++) {
				for (int x = 0; x < rect.w; x++) {
					ExactModelCoordinate elevCoord = cam->toElevationCoordinates(ScreenPoint(x, y));
					ModelCoordinate layerCoord = cg->toLayerCoordinates(elevCoord);

					if (prevLayerCoord != layerCoord) {
						data[x + rect.w * y] = edge_pixel;
					}
					prevLayerCoord = layerCoord;
				}
			}


			for (int x = 0; x < rect.w; x++) {
				for (int y = 0; y < rect.h; y++) {
					ExactModelCoordinate elevCoord = cam->toElevationCoordinates(ScreenPoint(x, y));
					ModelCoordinate layerCoord = cg->toLayerCoordinates(elevCoord);

					if (prevLayerCoord != layerCoord) {
						data[x + rect.w * y] = edge_pixel;
					}
					prevLayerCoord = layerCoord;
				}
			}

			m_zone_image =  m_renderbackend->createStaticImageFromRGBA((uint8_t*) data, rect.w, rect.h);
			delete data;
		}
		m_zone_image->render(rect);
	}

	void CameraZoneRenderer::setEnabled(bool enabled) {
		if (!enabled) {
			delete m_zone_image;
			m_zone_image = NULL;
		}
	}
}

#pragma once

#include <jni.h>

#include "indexer/map_object.hpp"
#include "map/place_page_info.hpp"

// Fills mapobject's metadata.
void InjectMetadata(JNIEnv * env, jclass clazz, jobject const mapObject, osm::MapObject const & src);

// Injects the core-computed Wikivoyage travel-guide URL (place_page::Info::GetWikivoyageUrl) as a
// synthetic FMD_WIKIVOYAGE metadata entry; no-op when the object has no Wikivoyage link.
void InjectWikivoyageUrl(JNIEnv * env, jclass clazz, jobject const mapObject, place_page::Info const & info);

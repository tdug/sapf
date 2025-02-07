//    SAPF - Sound As Pure Form
//    Copyright (C) 2019 James McCartney
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <https://www.gnu.org/licenses/>.

#import "makeImage.hpp"
#ifdef SAPF_COCOA
#import <Cocoa/Cocoa.h>

struct Bitmap
{
	NSBitmapImageRep* rep;
	unsigned char* data;
	int bytesPerRow;
};
#else
// TODO cross platform image creation
#endif // SAPF_COCOA

Bitmap* createBitmap(int width, int height)
{
#ifdef SAPF_COCOA
	Bitmap* bitmap = (Bitmap*)calloc(1, sizeof(Bitmap));
	bitmap->rep = 
			[[NSBitmapImageRep alloc] 
				initWithBitmapDataPlanes: nullptr
				pixelsWide: width
				pixelsHigh: height
				bitsPerSample: 8
				samplesPerPixel: 4
				hasAlpha: YES
				isPlanar: NO
				colorSpaceName: NSCalibratedRGBColorSpace
				bitmapFormat: NSAlphaNonpremultipliedBitmapFormat
				bytesPerRow: 0
				bitsPerPixel: 32
			];
	
	bitmap->data = [bitmap->rep bitmapData];
	bitmap->bytesPerRow = (int)[bitmap->rep bytesPerRow];
	return bitmap;
#else
        // TODO cross platform image creation
#endif // SAPF_COCOA
}

void setPixel(Bitmap* bitmap, int x, int y, int r, int g, int b, int a)
{
#ifdef SAPF_COCOA
	size_t index = bitmap->bytesPerRow * y + 4 * x;
	unsigned char* data = bitmap->data;
	
	data[index+0] = r;
	data[index+1] = g;
	data[index+2] = b;
	data[index+3] = a;
#else
        // TODO cross platform image creation
#endif // SAPF_COCOA
}

void fillRect(Bitmap* bitmap, int x, int y, int width, int height, int r, int g, int b, int a)
{
#ifdef SAPF_COCOA
	unsigned char* data = bitmap->data;
	for (int j = y; j < y + height; ++j) {
		size_t index = bitmap->bytesPerRow * j + 4 * x;
		for (int i = x; i < x + width; ++i) {
			data[index+0] = r;
			data[index+1] = g;
			data[index+2] = b;
			data[index+3] = a;
			index += 4;
		}
	}
#else
        // TODO cross platform image creation
#endif // SAPF_COCOA
}

void writeBitmap(Bitmap* bitmap, const char *path)
{
#ifdef SAPF_COCOA
	//NSData* data = [bitmap->rep TIFFRepresentation];
	//NSDictionary* properties = @{ NSImageCompressionFactor: @.9 };
	NSDictionary* properties = nullptr;
	NSData* data = [bitmap->rep representationUsingType: NSJPEGFileType properties: properties];
	NSString* nsstr = [NSString stringWithUTF8String: path];
	[data writeToFile: nsstr atomically: YES];
#else
        // TODO cross platform image creation
#endif // SAPF_COCOA
}

void freeBitmap(Bitmap* bitmap)
{
#ifdef SAPF_COCOA
	//[bitmap->rep release];
	free(bitmap);
#else
        // TODO cross platform image creation
#endif // SAPF_COCOA
}



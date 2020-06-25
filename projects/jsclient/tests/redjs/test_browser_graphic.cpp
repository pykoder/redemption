/*
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

Product name: redemption, a FLOSS RDP proxy
Copyright (C) Wallix 2010-2019
Author(s): Jonathan Poelen
*/

#include "test_only/test_framework/redemption_unit_tests.hpp"

#include "redjs/browser_graphic.hpp"
#include "red_emscripten/em_asm.hpp"
#include <emscripten/bind.h>

#define RED_JS_AUTO_TEST_CASE(name, func_params, ...)        \
    void test_ ## name ## _impl func_params;                 \
                                                             \
    EMSCRIPTEN_BINDINGS(test_ ## name ## _binding)           \
    {                                                        \
        emscripten::function(#name, test_ ## name ## _impl); \
    }                                                        \
                                                             \
    RED_AUTO_TEST_CASE(name)                                 \
    {                                                        \
        RED_EM_ASM({ Module. name (__VA_ARGS__); });         \
    }                                                        \
                                                             \
    void test_ ## name ## _impl func_params


#include "core/RDP/orders/RDPOrdersPrimaryOpaqueRect.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryMultiOpaqueRect.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryScrBlt.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryPatBlt.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryDestBlt.hpp"
#include "core/RDP/orders/RDPOrdersPrimaryMemBlt.hpp"
#include "core/RDP/orders/RDPOrdersSecondaryBmpCache.hpp"
#include "core/RDP/bitmapupdate.hpp"
#include "core/RDP/rdp_pointer.hpp"
#include "utils/bitmap.hpp"


RED_JS_AUTO_TEST_CASE(
    TestBrowserGraphics,
    (emscripten::val drawable),
    (() => {
        const Drawable = require("src/application/rdp_graphics").RDPGraphics;
        const { ImageData, createCanvas, Canvas } = require("node_modules/canvas");

        global.OffsreenCanvas = Canvas;
        global.ImageData = ImageData;

        const canvas = createCanvas(0, 0);
        canvas.style = {}; /* for cursor style */

        return new Drawable(canvas, Module);
    })()
) {
    auto canvas = drawable["_ecanvas"];
    auto to_data_url = [](emscripten::val& canvas){
        return canvas.call<std::string>("toDataURL");
    };

    Rect screen{0, 0, 400, 300};
    gdi::ColorCtx color_ctx(gdi::Depth::depth24(), nullptr);

    redjs::BrowserGraphic gd(drawable, 0, 0);
    gd.resize_canvas(ScreenInfo{screen.cx, screen.cy, BitsPerPixel(24)});


    // RDPOpaqueRect
    {
        gd.draw(
            RDPOpaqueRect(Rect(50, 50, 50, 50), RDPColor::from(0xFF0000)),
            screen, color_ctx);
        gd.draw(
            RDPOpaqueRect(Rect(-25, -25, 50, 50), RDPColor::from(0x00FF00)),
            screen, color_ctx);
        gd.draw(
            RDPOpaqueRect(Rect(375, 275, 50, 50), RDPColor::from(0x0000FF)),
            screen, color_ctx);
        gd.draw(
            RDPOpaqueRect(Rect(250, 150, 100, 100), RDPColor::from(0x00FFFF)),
            Rect(275, 175, 50, 50), color_ctx);
    }
    RED_CHECK(to_data_url(canvas) == "data:image/png;base64,"
        "iVBORw0KGgoAAAANSUhEUgAAAZAAAAEsCAYAAADtt+XCAAAABmJLR0QA/wD/AP+gvaeTA"
        "AAEb0lEQVR4nO3X0WkEMRAFwZHZ/FOWUzDN3gmZqgTm/TWzZs+eT1sfvwDAl/2cHgDAnQ"
        "QEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIB"
        "EQABIBASAREAASAQEgERAAEgEBIBEQABIBASAZM3MPj0CgPv4QABIBASAREAASAQEgERA"
        "AEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIB"
        "ASAREAASAQEgERAAEgEBIDkOT3gPfv0gJes0wMA/sQHAkAiIAAkAgJAIiAAJAICQCIgAC"
        "QCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJ"
        "AIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIg"
        "ACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkCyZmafH"
        "gHAfXwgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAk"
        "AiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiA"
        "AJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQC"
        "AkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAI"
        "iAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgAC"
        "QCAkDynB4A/A97n17wjrVOL7iHDwSAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgER"
        "AAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABI"
        "BASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEg"
        "ERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAZM3MPj0CgPv4QABIBASARE"
        "AASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEg"
        "EBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASA"
        "REAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAA"
        "EgEBIBEQABIntMDAHjf/sINHwgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJ"
        "AICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQ/AK"
        "RnwtW2ZkpUgAAAABJRU5ErkJggg=="_av);


    // RDPMultiOpaqueRect
    {
        StaticOutStream<256> out_stream;
        for (int i = 0; i < 5; ++i) {
            out_stream.out_sint16_le(25);
            out_stream.out_sint16_le(25);
            out_stream.out_sint16_le(50);
            out_stream.out_sint16_le(50);
        }

        InStream in_stream(out_stream.get_produced_bytes());

        gd.draw(
            RDPMultiOpaqueRect(0, 0, 300, 400, RDPColor::from(0xFFFF00),
            5, in_stream),
            screen, color_ctx);
    }
    RED_CHECK(to_data_url(canvas) == "data:image/png;base64,"
        "iVBORw0KGgoAAAANSUhEUgAAAZAAAAEsCAYAAADtt+XCAAAABmJLR0QA/wD/AP+gvaeTA"
        "AAErElEQVR4nO3XQa5aMRBFQXfE/rfsTKOMvo4wziNVG/AFJI561l57nTbHXwDgw37dHg"
        "DAMwkIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJA"
        "ICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAyay19vFX9vknPmLm9gKAf4YLBIBE"
        "QABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAAS"
        "AQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEhetwc8yt7n35g5/wbAG7hAAEgEBIBEQA"
        "BIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQ"
        "EgERAAEgEBIBEQABIBASAREAASAQEgGTWWvv2iLfY3/ExPmLm9gLgC7hAAEgEBIBEQABI"
        "BASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEg"
        "ERAAEgEBIBEQABIBASAREAASAQEgGTWWvv2iMfYvqofm7m9ADjMBQJAIiAAJAICQCIgAC"
        "QCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJ"
        "AIiAAJAICQCIgACQCAkAiIAAks9bat0fwh+3n+LGZ2wvgv+YCASAREAASAQEgERAAEgEB"
        "IBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASARE"
        "AASAQEgERAAEgEBIBEQAJLX7QH8Zeb8G3uffwP4ei4QABIBASAREAASAQEgERAAEgEBIB"
        "EQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAA"
        "SAQEgERAAEgEBIJm11r49Ani+/SX/JDO3FzyHCwSAREAASAQEgERAAEgEBIBEQABIBASA"
        "REAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAA"
        "EgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBA"
        "SAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAZNZa+/YIAJ7"
        "HBQJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAIC"
        "QCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiI"
        "AAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJA"
        "ICQCIgACQCAkAiIAAkAgJA8ro9AID32x94wwUCQCIgACQCAkAiIAAkAgJAIiAAJAICQCI"
        "gACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAk"
        "AgJAIiAAJL8B/yIZUA55UcwAAAAASUVORK5CYII="_av);


    // RDPScrBlt
    {
        gd.draw(RDPScrBlt(Rect(100, 75, 200, 150), 0x55, 100, 75), screen);
        gd.draw(RDPScrBlt(screen, 0xCC, screen.cx/2, screen.cy/2), screen);
    }
    RED_CHECK(to_data_url(canvas) == "data:image/png;base64,"
        "iVBORw0KGgoAAAANSUhEUgAAAZAAAAEsCAYAAADtt+XCAAAABmJLR0QA/wD/AP+gvaeTA"
        "AAEmUlEQVR4nO3XQWrDQBBFQU3Q/a/c2SaLQHhEGlupusB8G5tHr5mZg5ex1to9AeBXPn"
        "YPAOA9CQgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYA"
        "AkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICADJuXvAO1nr+jdmrn/jjs8BPJ8L"
        "BIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAR"
        "EAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAE"
        "gEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBAS"
        "AREAASAQEgERAAEgEBIBkHccxu0fAq5rx93gla63dE/jCBQJAIiAAJAICQCIgACQCAkAi"
        "IAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJ"
        "AICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAk"
        "AiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAcu4"
        "eANXsHgD/nAsEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQE"
        "gERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASM7dA/huZq5/ZK3r3"
        "7jDHd8V8CMXCACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICA"
        "CJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJCcuwewwczuBTzQWte"
        "/ccdP947P8RQuEAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAR"
        "EAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAE"
        "gEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBAS"
        "AREAASAQEgERAAEgEBIBEQABIBASAREACSdRzH7B4BwPtxgQCQCAgAiYAAkAgIAImAAJA"
        "ICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgA"
        "iYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAA"
        "JAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQnL"
        "sHAPD35oY3XCAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiA"
        "AJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJA8gn8MhhaXHR11QAA"
        "AABJRU5ErkJggg=="_av);


    // RDPPatBlt
    {
        gd.draw(RDPPatBlt(screen, 0x55, RDPColor(), RDPColor(), RDPBrush()), screen, color_ctx);
    }
    RED_CHECK(to_data_url(canvas) == "data:image/png;base64,"
        "iVBORw0KGgoAAAANSUhEUgAAAZAAAAEsCAYAAADtt+XCAAAABmJLR0QA/wD/AP+gvaeTA"
        "AAElUlEQVR4nO3XQXKDQAwAQTbF/7+sXONDLlOBNU73ByQM5Smt4zjm4G3MeB3AM3ztXg"
        "CAZxIQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASA"
        "REAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAknP3Ak8yc/2Mta6fccdzAJ/PBQJA"
        "IiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgA"
        "CQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAg"
        "JAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCI"
        "gACQCAkAiIAAkAgJAsmZmdi8B72qttXsFfvB39V5cIAAkAgJAIiAAJAICQCIgACQCAkAi"
        "IAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJ"
        "AICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAk"
        "AiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAk5+4FoFq"
        "7F4B/zgUCQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIg"
        "ACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJOfuBXi11rp+yMz1M+5wx"
        "28F/MoFAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIA"
        "AkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACTn7gXYYK3dG/CBZq6fcce"
        "ne8dzfAoXCACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJ"
        "gACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAk"
        "AgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICA"
        "CJgACQCAgAiYAAkAgIAImAAJAICADJmpnZvQQAz+MCASAREAASAQEgERAAEgEBIBEQABI"
        "BASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEg"
        "ERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQA"
        "BIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASA5dy8AwN"
        "9bN8xwgQCQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAg"
        "AiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICADJNwoDG1epulrBAAAAAElF"
        "TkSuQmCC"_av);


    // RDPDestBlt
    {
        // restore previous image
        gd.draw(RDPDestBlt(screen, 0x55), screen);
    }
    RED_CHECK(to_data_url(canvas) == "data:image/png;base64,"
        "iVBORw0KGgoAAAANSUhEUgAAAZAAAAEsCAYAAADtt+XCAAAABmJLR0QA/wD/AP+gvaeTA"
        "AAEmUlEQVR4nO3XQWrDQBBFQU3Q/a/c2SaLQHhEGlupusB8G5tHr5mZg5ex1to9AeBXPn"
        "YPAOA9CQgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYA"
        "AkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICADJuXvAO1nr+jdmrn/jjs8BPJ8L"
        "BIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAR"
        "EAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAE"
        "gEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBAS"
        "AREAASAQEgERAAEgEBIBkHccxu0fAq5rx93gla63dE/jCBQJAIiAAJAICQCIgACQCAkAi"
        "IAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJ"
        "AICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAk"
        "AiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAcu4"
        "eANXsHgD/nAsEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQE"
        "gERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASM7dA/huZq5/ZK3r3"
        "7jDHd8V8CMXCACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICA"
        "CJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJCcuwewwczuBTzQWte"
        "/ccdP947P8RQuEAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAR"
        "EAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAE"
        "gEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBAS"
        "AREAASAQEgERAAEgEBIBEQABIBASAREACSdRzH7B4BwPtxgQCQCAgAiYAAkAgIAImAAJA"
        "ICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgA"
        "iYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAA"
        "JAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQnL"
        "sHAPD35oY3XCAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiA"
        "AJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJA8gn8MhhaXHR11QAA"
        "AABJRU5ErkJggg=="_av);


    BGRPalette palette332(BGRPalette::classic_332());

    uint8_t raw24[] = {
        0x22, 0x17, 0x48, 0xc7, 0xcd, 0xc4, 0xad, 0xf8, 0x61, 0x6f, 0x32, 0xd6, 0x13, 0x61, 0xee,
        0xb2, 0x7b, 0x81, 0x0f, 0x66, 0x22, 0x17, 0x48, 0xc7, 0xcd, 0xc4, 0xad, 0xf8, 0x61, 0x6f,
        0x32, 0xd6, 0x13, 0x61, 0xee, 0xb2, 0x7b, 0x81, 0x0f, 0x66, 0x22, 0x17, 0x48, 0xc7, 0xcd,
        0xc4, 0xad, 0xf8, 0x61, 0x6f, 0x32, 0xd6, 0x13, 0x61, 0xee, 0xb2, 0x7b, 0x81, 0x0f, 0x66,
    };

    Bitmap bmp24(BitsPerPixel{24}, BitsPerPixel{24}, &palette332, 4, 5, raw24, sizeof(raw24));


    // RDPBitmapData
    {
        RDPBitmapData d;
        d.dest_right = bmp24.cx();
        d.dest_bottom = bmp24.cy();
        gd.draw(d, bmp24);

        d.dest_left = d.dest_right;
        d.dest_top = d.dest_bottom;
        d.dest_right *= 2;
        d.dest_bottom *= 2;
        gd.draw(d, bmp24);
    }
    RED_CHECK(to_data_url(canvas) == "data:image/png;base64,"
        "iVBORw0KGgoAAAANSUhEUgAAAZAAAAEsCAYAAADtt+XCAAAABmJLR0QA/wD/AP+gvaeTA"
        "AAFTElEQVR4nO3dsYpUZxiA4f8s0w8IC2FvwUJIOpsgAS/AKggW5gZiYye5hiVWKdw0Xs"
        "MUlrkPQQQbbbyDY2schOHFnX9Hn6c8zflmmOHlnw/OLD//+Xi98/LpuPj3p/Hh0fPxz8e"
        "/BvMsyzJ7BICDbF7fuhjvtpdjd/523L56NnseAE7E2dVvD8bvv/497m/fj1f3XsyeB4AT"
        "cXbvxf1x6+LV2F6+HucPd7PnAeBEbB7utuPZ7ny8uX01tnf/mz0PACdiWdd1/fzCL0/+G"
        "Jbq81iiA6di8+UFS3UADnH25QVLdQAOsRcQS3UADrH3E5alOgCH2FuiM5clOnAq9n7CAo"
        "BDCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAyd7/gfB"
        "1x3jS+jEeru+J8cC34AQCQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJA"
        "IiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgA"
        "CQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAg"
        "JAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQLKMMdbZQ8BNta6+HjfJsiyzR+AzTiA"
        "AJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQC"
        "AkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAI"
        "iAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgAC"
        "QCAkAiIAAkAgJAIiAAJJvZA0C1zh4AfnBOIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJ"
        "AIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIg"
        "ACQCAkCymT0A/7eu6/XfZFmu/x7HcIz3CvgqJxAAEgEBIBEQABIBASAREAASAQEgERAAE"
        "gEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBAS"
        "AREAASAQEg2cwegAnWdfYEfIeW5frvcYyP7jFex/fCCQSAREAASAQEgERAAEgEBIBEQAB"
        "IBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQE"
        "gERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQ"
        "ABIBASAREAASAQEgERAAEgEBIBEQABIBASAREAASAQEgERAAEgEBIBEQABIBASAZBljrL"
        "OHAOD0OIEAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAg"
        "IAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJ"
        "gACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAkAgIAImAAJAICACJgACQCAgAiYAAk"
        "AgIAImAAJAICACJgACQCAgAiYAAkGxmDwDAt7ce4R5OIAAkAgJAIiAAJAICQCIgACQCAk"
        "AiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiAAJAICQCIgACQCAkAiIAAkAgJAIiA"
        "AJAICQCIgACQCAkDyCb+xVEylVB4JAAAAAElFTkSuQmCC"_av);

    // RDPBmpCache / RDPMemBlt
    {
        gd.draw(RDPBmpCache(bmp24, 0, 0, false, false));
        gd.draw(RDPMemBlt(0, Rect(10, 10, bmp24.cx(), bmp24.cy()), 0xCC, 0, 0, 0), screen);
        gd.draw(RDPMemBlt(0, Rect(0, 10, bmp24.cx(), bmp24.cy()), 0x22, 0, 0, 0), screen);
        gd.draw(RDPMemBlt(0, Rect(0, 20, bmp24.cx(), bmp24.cy()), 0x55, 0, 0, 0), screen);
    }
    RED_CHECK(to_data_url(canvas) == "x"_av);


    // Pointer
    gd.set_pointer(0, edit_pointer(), gdi::GraphicApi::SetPointerMode::Insert);
    RED_CHECK(canvas["style"]["cursor"].as<std::string>() == "x"_av);
}

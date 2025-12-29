#include "renderer.hpp"
#include "common.hpp"
#include "config.hpp"

#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <algorithm>
#include <cmath>
#include <malloc.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

namespace waul {

Buffer Renderer::buf;
wl_shm* Renderer::shm_ref = nullptr;

static int create_shm_file(size_t size) {
    int fd = memfd_create("waul-shm", MFD_CLOEXEC);
    if (fd < 0) return -1;
    if (ftruncate(fd, size) < 0) { close(fd); return -1; }
    return fd;
}

void Renderer::init(wl_shm* shm) {
    shm_ref = shm;
}

void Renderer::cleanup() {
    if (buf.wlbuf) wl_buffer_destroy(buf.wlbuf);
    if (buf.fd != -1) close(buf.fd);
    buf.wlbuf = nullptr;
    buf.fd = -1;
}

void Renderer::resize_buffer(int w, int h) {
    cleanup();
    buf.w = w; buf.h = h;
    int stride = w * 4;
    buf.size = stride * h;
    buf.fd = create_shm_file(buf.size);
    
    if (buf.fd >= 0) {
        wl_shm_pool* pool = wl_shm_create_pool(shm_ref, buf.fd, buf.size);
        buf.wlbuf = wl_shm_pool_create_buffer(pool, 0, w, h, stride, WL_SHM_FORMAT_XRGB8888);
        wl_shm_pool_destroy(pool);
    }
}

static uint32_t blend(uint32_t c1, uint32_t c2, float factor) {
    int r1 = (c1 >> 16) & 0xFF, g1 = (c1 >> 8) & 0xFF, b1 = c1 & 0xFF;
    int r2 = (c2 >> 16) & 0xFF, g2 = (c2 >> 8) & 0xFF, b2 = c2 & 0xFF;
    
    int r = r1 + (r2 - r1) * factor;
    int g = g1 + (g2 - g1) * factor;
    int b = b1 + (b2 - b1) * factor;
    
    return (0xFF << 24) | (r << 16) | (g << 8) | b;
}

static uint32_t mix_alpha(int r, int g, int b, int a, uint32_t bg) {
    if (a == 0) return bg;
    if (a == 255) return (0xFF << 24) | (r << 16) | (g << 8) | b;
    return blend(bg, (0xFF << 24) | (r << 16) | (g << 8) | b, a / 255.0f);
}

void Renderer::draw(const std::string& path, wl_surface* surf) {
    if (buf.fd == -1) return;

    Config::load();
    const auto& cfg = Config::get();

    void* data = mmap(nullptr, buf.size, PROT_READ | PROT_WRITE, MAP_SHARED, buf.fd, 0);
    if (data == MAP_FAILED) return;

    uint32_t* pixels = (uint32_t*)data;
    uint32_t bg_color = (0xFF << 24) | (cfg.bg[0] << 16) | (cfg.bg[1] << 8) | cfg.bg[2];
    
    uint32_t border_color = (0xFF << 24) | (cfg.bc[0] << 16) | (cfg.bc[1] << 8) | cfg.bc[2];
    if (cfg.bc[3] < 255) border_color = mix_alpha(cfg.bc[0], cfg.bc[1], cfg.bc[2], cfg.bc[3], bg_color);

    int cx = cfg.m[1]; 
    int cy = cfg.m[0]; 
    int cw = buf.w - cfg.m[1] - cfg.m[3]; 
    int ch = buf.h - cfg.m[0] - cfg.m[2]; 

    int iw = 0, ih = 0, ic = 0;
    uint8_t* img = nullptr;
    if (!path.empty()) {
        img = stbi_load(path.c_str(), &iw, &ih, &ic, 4);
    }

    float scale = 0;
    int ox = 0, oy = 0;

    if (img) {
        int inner_w = cw - cfg.bw[1] - cfg.bw[3];
        int inner_h = ch - cfg.bw[0] - cfg.bw[2];
        scale = std::max((float)inner_w / iw, (float)inner_h / ih);
        int sw = iw * scale;
        int sh = ih * scale;
        ox = cx + cfg.bw[1] + (inner_w - sw) / 2;
        oy = cy + cfg.bw[0] + (inner_h - sh) / 2;
    }

    for (int y = 0; y < buf.h; y++) {
        for (int x = 0; x < buf.w; x++) {
            bool is_margin = (x < cx || x >= cx + cw || y < cy || y >= cy + ch);
            uint32_t final_pixel = bg_color;

            if (!is_margin) {
                int rx = x - cx, ry = y - cy;
                int rad = 0;
                float dist = 0;

                // Corner Check
                if (rx < cfg.br[0] && ry < cfg.br[0]) { 
                    rad = cfg.br[0]; 
                    dist = sqrt(pow(cfg.br[0]-rx-0.5f, 2) + pow(cfg.br[0]-ry-0.5f, 2)); 
                } 
                else if (rx >= cw - cfg.br[1] && ry < cfg.br[1]) { 
                    rad = cfg.br[1]; 
                    dist = sqrt(pow(rx-(cw-cfg.br[1])+0.5f, 2) + pow(cfg.br[1]-ry-0.5f, 2)); 
                } 
                else if (rx >= cw - cfg.br[2] && ry >= ch - cfg.br[2]) { 
                    rad = cfg.br[2]; 
                    dist = sqrt(pow(rx-(cw-cfg.br[2])+0.5f, 2) + pow(ry-(ch-cfg.br[2])+0.5f, 2)); 
                } 
                else if (rx < cfg.br[3] && ry >= ch - cfg.br[3]) { 
                    rad = cfg.br[3]; 
                    dist = sqrt(pow(cfg.br[3]-rx-0.5f, 2) + pow(ry-(ch-cfg.br[3])+0.5f, 2)); 
                }

                uint32_t content_pixel = bg_color;
                
                // Determine content pixel
                bool is_border = (y < cy + cfg.bw[0] || y >= cy + ch - cfg.bw[2] || x < cx + cfg.bw[1] || x >= cx + cw - cfg.bw[3]);
                
                if (is_border) {
                    content_pixel = border_color;
                } else if (img) {
                    int sx = (x - ox) / scale;
                    int sy = (y - oy) / scale;
                    if (sx >= 0 && sy >= 0 && sx < iw && sy < ih) {
                        int i = (sy * iw + sx) * 4;
                        content_pixel = (0xFF << 24) | (img[i] << 16) | (img[i+1] << 8) | img[i+2];
                    } else {
                        content_pixel = border_color; // Fallback if scaling leaves gaps
                    }
                } else {
                    content_pixel = border_color;
                }

                if (rad > 0) {
                    // Pick border width for this corner
                    int bw = 0;
                    if (rx < cfg.br[0] && ry < cfg.br[0])               bw = std::max(cfg.bw[0], cfg.bw[1]); // TL
                    else if (rx >= cw - cfg.br[1] && ry < cfg.br[1])   bw = std::max(cfg.bw[0], cfg.bw[3]); // TR
                    else if (rx >= cw - cfg.br[2] && ry >= ch - cfg.br[2]) bw = std::max(cfg.bw[2], cfg.bw[3]); // BR
                    else if (rx < cfg.br[3] && ry >= ch - cfg.br[3])   bw = std::max(cfg.bw[2], cfg.bw[1]); // BL

                    int outer_rad = rad;
                    int inner_rad = std::max(0, outer_rad - bw);
                    float dx = 0.0f, dy = 0.0f;

                    if (rx < rad && ry < rad) { // TL
                        dx = std::max(0.0f, float(rad - rx - 0.5f));
                        dy = std::max(0.0f, float(rad - ry - 0.5f));
                    }
                    else if (rx >= cw - rad && ry < rad) { // TR
                        dx = std::max(0.0f, float(rx - (cw - rad) + 0.5f));
                        dy = std::max(0.0f, float(rad - ry - 0.5f));
                    }
                    else if (rx >= cw - rad && ry >= ch - rad) { // BR
                        dx = std::max(0.0f, float(rx - (cw - rad) + 0.5f));
                        dy = std::max(0.0f, float(ry - (ch - rad) + 0.5f));
                    }
                    else if (rx < rad && ry >= ch - rad) { // BL
                        dx = std::max(0.0f, float(rad - rx - 0.5f));
                        dy = std::max(0.0f, float(ry - (ch - rad) + 0.5f));
                    }

                    float dist = sqrt(dx * dx + dy * dy) - 0.5f;

                    if (dist > outer_rad) {
                        final_pixel = bg_color;
                    } 
                    else if (dist > outer_rad - 1.0f) {
                        float t = dist - (outer_rad - 1.0f);
                        final_pixel = blend(border_color, bg_color, t);
                    }
                    else if (dist > inner_rad) {
                        final_pixel = border_color;
                    }
                    else if (dist > inner_rad - 1.0f) {
                        float t = dist - (inner_rad - 1.0f);
                        final_pixel = blend(content_pixel, border_color, t);
                    }
                    else {
                        final_pixel = content_pixel;
                    }
                } else {
                    final_pixel = content_pixel;
                }
            }
            pixels[y * buf.w + x] = final_pixel;
        }
    }

    if (img) stbi_image_free(img);
    
    malloc_trim(0); 

    munmap(data, buf.size);

    wl_surface_attach(surf, buf.wlbuf, 0, 0);
    wl_surface_damage(surf, 0, 0, buf.w, buf.h);
    wl_surface_commit(surf);
}

}
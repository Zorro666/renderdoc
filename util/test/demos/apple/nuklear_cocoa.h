/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2019-2021 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

/*
 * Based on nuklear_glfw_gl3 from Nuklear - 1.32.0 - public domain
 */

/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_COCOA_H_
#define NK_COCOA_H_

#include "apple_cocoa.h"

enum nk_cocoa_init_state
{
  NK_COCOA_DEFAULT,
  NK_COCOA_INSTALL_CALLBACKS
};

NK_API struct nk_context *nk_cocoa_init(COCOAwindow *win, enum nk_cocoa_init_state);
NK_API void nk_cocoa_shutdown(void);
NK_API void nk_cocoa_font_stash_begin(struct nk_font_atlas **atlas);
NK_API void nk_cocoa_font_stash_end(void);
NK_API void nk_cocoa_new_frame(void);
NK_API void nk_cocoa_render(enum nk_anti_aliasing, int max_vertex_buffer, int max_element_buffer);

NK_API void nk_cocoa_device_destroy(void);
NK_API void nk_cocoa_device_create(void);

NK_API void nk_cocoa_char_callback(COCOAwindow *win, unsigned int codepoint);
NK_API void nk_cocoa_scroll_callback(COCOAwindow *win, double xoff, double yoff);
NK_API void nk_cocoa_mouse_button_callback(COCOAwindow *win, int button, int action, int mods);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_COCOA_IMPLEMENTATION

#ifndef NK_COCOA_TEXT_MAX
#define NK_COCOA_TEXT_MAX 256
#endif
#ifndef NK_COCOA_DOUBLE_CLICK_LO
#define NK_COCOA_DOUBLE_CLICK_LO 0.02
#endif
#ifndef NK_COCOA_DOUBLE_CLICK_HI
#define NK_COCOA_DOUBLE_CLICK_HI 0.2
#endif

struct nk_cocoa_device
{
  struct nk_buffer cmds;
  struct nk_draw_null_texture null;
  GLuint vbo, vao, ebo;
  GLuint prog;
  GLuint vert_shdr;
  GLuint frag_shdr;
  GLint attrib_pos;
  GLint attrib_uv;
  GLint attrib_col;
  GLint uniform_tex;
  GLint uniform_proj;
  GLuint font_tex;
};

struct nk_cocoa_vertex
{
  float position[2];
  float uv[2];
  nk_byte col[4];
};

static struct nk_cocoa
{
  COCOAwindow *win;
  int width, height;
  int display_width, display_height;
  struct nk_cocoa_device ogl;
  struct nk_context ctx;
  struct nk_font_atlas atlas;
  struct nk_vec2 fb_scale;
  unsigned int text[NK_COCOA_TEXT_MAX];
  int text_len;
  struct nk_vec2 scroll;
  double last_button_click;
  int is_double_click_down;
  struct nk_vec2 double_click_pos;
} nk_cocoa;

#ifdef __APPLE__
#define NK_SHADER_VERSION "#version 150\n"
#else
#define NK_SHADER_VERSION "#version 300 es\n"
#endif

NK_API void nk_cocoa_device_create(void)
{
  GLint status;
  static const GLchar *vertex_shader = NK_SHADER_VERSION
      "uniform mat4 ProjMtx;\n"
      "in vec2 Position;\n"
      "in vec2 TexCoord;\n"
      "in vec4 Color;\n"
      "out vec2 Frag_UV;\n"
      "out vec4 Frag_Color;\n"
      "void main() {\n"
      "   Frag_UV = TexCoord;\n"
      "   Frag_Color = Color;\n"
      "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
      "}\n";
  static const GLchar *fragment_shader = NK_SHADER_VERSION
      "precision mediump float;\n"
      "uniform sampler2D Texture;\n"
      "in vec2 Frag_UV;\n"
      "in vec4 Frag_Color;\n"
      "out vec4 Out_Color;\n"
      "void main(){\n"
      "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
      "}\n";

  struct nk_cocoa_device *dev = &nk_cocoa.ogl;
  nk_buffer_init_default(&dev->cmds);
  dev->prog = glCreateProgram();
  dev->vert_shdr = glCreateShader(GL_VERTEX_SHADER);
  dev->frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
  glShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);
  glCompileShader(dev->vert_shdr);
  glCompileShader(dev->frag_shdr);
  glGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);
  assert(status == GL_TRUE);
  glGetShaderiv(dev->frag_shdr, GL_COMPILE_STATUS, &status);
  assert(status == GL_TRUE);
  glAttachShader(dev->prog, dev->vert_shdr);
  glAttachShader(dev->prog, dev->frag_shdr);
  glLinkProgram(dev->prog);
  glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
  assert(status == GL_TRUE);

  dev->uniform_tex = glGetUniformLocation(dev->prog, "Texture");
  dev->uniform_proj = glGetUniformLocation(dev->prog, "ProjMtx");
  dev->attrib_pos = glGetAttribLocation(dev->prog, "Position");
  dev->attrib_uv = glGetAttribLocation(dev->prog, "TexCoord");
  dev->attrib_col = glGetAttribLocation(dev->prog, "Color");

  {
    /* buffer setup */
    GLsizei vs = sizeof(struct nk_cocoa_vertex);
    size_t vp = offsetof(struct nk_cocoa_vertex, position);
    size_t vt = offsetof(struct nk_cocoa_vertex, uv);
    size_t vc = offsetof(struct nk_cocoa_vertex, col);

    glGenBuffers(1, &dev->vbo);
    glGenBuffers(1, &dev->ebo);
    glGenVertexArrays(1, &dev->vao);

    glBindVertexArray(dev->vao);
    glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

    glEnableVertexAttribArray((GLuint)dev->attrib_pos);
    glEnableVertexAttribArray((GLuint)dev->attrib_uv);
    glEnableVertexAttribArray((GLuint)dev->attrib_col);

    glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void *)vp);
    glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void *)vt);
    glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void *)vc);
  }

  glBindTexture(GL_TEXTURE_2D, 0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

NK_INTERN void nk_cocoa_device_upload_atlas(const void *image, int width, int height)
{
  struct nk_cocoa_device *dev = &nk_cocoa.ogl;
  glGenTextures(1, &dev->font_tex);
  glBindTexture(GL_TEXTURE_2D, dev->font_tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, image);
}

NK_API void nk_cocoa_device_destroy(void)
{
  struct nk_cocoa_device *dev = &nk_cocoa.ogl;
  glDetachShader(dev->prog, dev->vert_shdr);
  glDetachShader(dev->prog, dev->frag_shdr);
  glDeleteShader(dev->vert_shdr);
  glDeleteShader(dev->frag_shdr);
  glDeleteProgram(dev->prog);
  glDeleteTextures(1, &dev->font_tex);
  glDeleteBuffers(1, &dev->vbo);
  glDeleteBuffers(1, &dev->ebo);
  nk_buffer_free(&dev->cmds);
}

NK_API void nk_cocoa_render(enum nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer)
{
  struct nk_cocoa_device *dev = &nk_cocoa.ogl;
  struct nk_buffer vbuf, ebuf;
  GLfloat ortho[4][4] = {
      {2.0f, 0.0f, 0.0f, 0.0f},
      {0.0f, -2.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, -1.0f, 0.0f},
      {-1.0f, 1.0f, 0.0f, 1.0f},
  };
  ortho[0][0] /= (GLfloat)nk_cocoa.width;
  ortho[1][1] /= (GLfloat)nk_cocoa.height;

  /* setup global state */
  glEnable(GL_BLEND);
  glBlendEquation(GL_FUNC_ADD);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_SCISSOR_TEST);
  glActiveTexture(GL_TEXTURE0);

  /* setup program */
  glUseProgram(dev->prog);
  glUniform1i(dev->uniform_tex, 0);
  glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
  glViewport(0, 0, (GLsizei)nk_cocoa.display_width, (GLsizei)nk_cocoa.display_height);
  {
    /* convert from command queue into draw list and draw to screen */
    const struct nk_draw_command *cmd;
    void *vertices, *elements;
    const nk_draw_index *offset = NULL;

    /* allocate vertex and element buffer */
    glBindVertexArray(dev->vao);
    glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

    glBufferData(GL_ARRAY_BUFFER, max_vertex_buffer, NULL, GL_STREAM_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_element_buffer, NULL, GL_STREAM_DRAW);

    /* load draw vertices & elements directly into vertex + element buffer */
    vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
    elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
    {
      /* fill convert configuration */
      struct nk_convert_config config;
      static const struct nk_draw_vertex_layout_element vertex_layout[] = {
          {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_cocoa_vertex, position)},
          {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_cocoa_vertex, uv)},
          {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_cocoa_vertex, col)},
          {NK_VERTEX_LAYOUT_END}};
      NK_MEMSET(&config, 0, sizeof(config));
      config.vertex_layout = vertex_layout;
      config.vertex_size = sizeof(struct nk_cocoa_vertex);
      config.vertex_alignment = NK_ALIGNOF(struct nk_cocoa_vertex);
      config.null = dev->null;
      config.circle_segment_count = 22;
      config.curve_segment_count = 22;
      config.arc_segment_count = 22;
      config.global_alpha = 1.0f;
      config.shape_AA = AA;
      config.line_AA = AA;

      /* setup buffers to load vertices and elements */
      nk_buffer_init_fixed(&vbuf, vertices, (size_t)max_vertex_buffer);
      nk_buffer_init_fixed(&ebuf, elements, (size_t)max_element_buffer);
      nk_convert(&nk_cocoa.ctx, &dev->cmds, &vbuf, &ebuf, &config);
    }
    glUnmapBuffer(GL_ARRAY_BUFFER);
    glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

    /* iterate over and execute each draw command */
    nk_draw_foreach(cmd, &nk_cocoa.ctx, &dev->cmds)
    {
      if(!cmd->elem_count)
        continue;
      glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
      glScissor((GLint)(cmd->clip_rect.x * nk_cocoa.fb_scale.x),
                (GLint)((nk_cocoa.height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) *
                        nk_cocoa.fb_scale.y),
                (GLint)(cmd->clip_rect.w * nk_cocoa.fb_scale.x),
                (GLint)(cmd->clip_rect.h * nk_cocoa.fb_scale.y));
      glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
      offset += cmd->elem_count;
    }
    nk_clear(&nk_cocoa.ctx);
  }

  /* default OpenGL state */
  glUseProgram(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  glDisable(GL_BLEND);
  glDisable(GL_SCISSOR_TEST);
}

NK_API void nk_cocoa_char_callback(COCOAwindow *win, unsigned int codepoint)
{
  (void)win;
  if(nk_cocoa.text_len < NK_COCOA_TEXT_MAX)
    nk_cocoa.text[nk_cocoa.text_len++] = codepoint;
}

NK_API void nk_cocoa_scroll_callback(COCOAwindow *win, double xoff, double yoff)
{
  (void)win;
  (void)xoff;
  nk_cocoa.scroll.x += (float)xoff;
  nk_cocoa.scroll.y += (float)yoff;
}

NK_API void nk_cocoa_mouse_button_callback(COCOAwindow *window, int button, int action, int mods)
{
  double x, y;
  if(button != COCOA_MOUSE_BUTTON_LEFT)
    return;
  COCOA_GetMousePosition(window, &x, &y);
  if(action == COCOA_PRESS)
  {
    double dt = COCOA_GetTime() - nk_cocoa.last_button_click;
    if(dt > NK_COCOA_DOUBLE_CLICK_LO && dt < NK_COCOA_DOUBLE_CLICK_HI)
    {
      nk_cocoa.is_double_click_down = nk_true;
      nk_cocoa.double_click_pos = nk_vec2((float)x, (float)y);
    }
    nk_cocoa.last_button_click = COCOA_GetTime();
  }
  else
    nk_cocoa.is_double_click_down = nk_false;
}

NK_API struct nk_context *nk_cocoa_init(COCOAwindow *win, enum nk_cocoa_init_state init_state)
{
  nk_cocoa.win = win;
  if(init_state == NK_COCOA_INSTALL_CALLBACKS)
  {
    COCOA_SetCharacterCallback(win, nk_cocoa_char_callback);
    COCOA_SetScrollCallback(win, nk_cocoa_scroll_callback);
    COCOA_SetMouseButtonCallback(win, nk_cocoa_mouse_button_callback);
  }
  nk_init_default(&nk_cocoa.ctx, 0);
  nk_cocoa.ctx.clip.userdata = nk_handle_ptr(0);
  nk_cocoa.last_button_click = 0;
  nk_cocoa_device_create();

  nk_cocoa.is_double_click_down = nk_false;
  nk_cocoa.double_click_pos = nk_vec2(0, 0);

  return &nk_cocoa.ctx;
}

NK_API void nk_cocoa_font_stash_begin(struct nk_font_atlas **atlas)
{
  nk_font_atlas_init_default(&nk_cocoa.atlas);
  nk_font_atlas_begin(&nk_cocoa.atlas);
  *atlas = &nk_cocoa.atlas;
}

NK_API void nk_cocoa_font_stash_end(void)
{
  const void *image;
  int w, h;
  image = nk_font_atlas_bake(&nk_cocoa.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
  nk_cocoa_device_upload_atlas(image, w, h);
  nk_font_atlas_end(&nk_cocoa.atlas, nk_handle_id((int)nk_cocoa.ogl.font_tex), &nk_cocoa.ogl.null);
  if(nk_cocoa.atlas.default_font)
    nk_style_set_font(&nk_cocoa.ctx, &nk_cocoa.atlas.default_font->handle);
}

NK_API void nk_cocoa_new_frame(void)
{
  int i;
  double x, y;
  struct nk_context *ctx = &nk_cocoa.ctx;
  COCOAwindow *win = nk_cocoa.win;

  COCOA_GetWindowSize(win, &nk_cocoa.width, &nk_cocoa.height);
  COCOA_GetFrameBufferSize(win, &nk_cocoa.display_width, &nk_cocoa.display_height);
  nk_cocoa.fb_scale.x = (float)nk_cocoa.display_width / (float)nk_cocoa.width;
  nk_cocoa.fb_scale.y = (float)nk_cocoa.display_height / (float)nk_cocoa.height;

  nk_input_begin(ctx);
  for(i = 0; i < nk_cocoa.text_len; ++i)
    nk_input_unicode(ctx, nk_cocoa.text[i]);

  nk_input_key(ctx, NK_KEY_DEL, COCOA_GetKeyState(win, COCOA_KEY_DELETE) == COCOA_PRESS);
  nk_input_key(ctx, NK_KEY_ENTER, COCOA_GetKeyState(win, COCOA_KEY_ENTER) == COCOA_PRESS);
  nk_input_key(ctx, NK_KEY_TAB, COCOA_GetKeyState(win, COCOA_KEY_TAB) == COCOA_PRESS);
  nk_input_key(ctx, NK_KEY_BACKSPACE, COCOA_GetKeyState(win, COCOA_KEY_BACKSPACE) == COCOA_PRESS);
  nk_input_key(ctx, NK_KEY_UP, COCOA_GetKeyState(win, COCOA_KEY_UP) == COCOA_PRESS);
  nk_input_key(ctx, NK_KEY_DOWN, COCOA_GetKeyState(win, COCOA_KEY_DOWN) == COCOA_PRESS);
  nk_input_key(ctx, NK_KEY_TEXT_START, COCOA_GetKeyState(win, COCOA_KEY_HOME) == COCOA_PRESS);
  nk_input_key(ctx, NK_KEY_TEXT_END, COCOA_GetKeyState(win, COCOA_KEY_END) == COCOA_PRESS);
  nk_input_key(ctx, NK_KEY_SCROLL_START, COCOA_GetKeyState(win, COCOA_KEY_HOME) == COCOA_PRESS);
  nk_input_key(ctx, NK_KEY_SCROLL_END, COCOA_GetKeyState(win, COCOA_KEY_END) == COCOA_PRESS);
  nk_input_key(ctx, NK_KEY_SCROLL_DOWN, COCOA_GetKeyState(win, COCOA_KEY_PAGE_DOWN) == COCOA_PRESS);
  nk_input_key(ctx, NK_KEY_SCROLL_UP, COCOA_GetKeyState(win, COCOA_KEY_PAGE_UP) == COCOA_PRESS);
  nk_input_key(ctx, NK_KEY_SHIFT, COCOA_GetKeyState(win, COCOA_KEY_LEFT_SHIFT) == COCOA_PRESS ||
                                      COCOA_GetKeyState(win, COCOA_KEY_RIGHT_SHIFT) == COCOA_PRESS);

  if(COCOA_GetKeyState(win, COCOA_KEY_LEFT_CONTROL) == COCOA_PRESS ||
     COCOA_GetKeyState(win, COCOA_KEY_RIGHT_CONTROL) == COCOA_PRESS)
  {
    nk_input_key(ctx, NK_KEY_COPY, COCOA_GetKeyState(win, COCOA_KEY_C) == COCOA_PRESS);

    nk_input_key(ctx, NK_KEY_PASTE, COCOA_GetKeyState(win, COCOA_KEY_V) == COCOA_PRESS);
    nk_input_key(ctx, NK_KEY_CUT, COCOA_GetKeyState(win, COCOA_KEY_X) == COCOA_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_UNDO, COCOA_GetKeyState(win, COCOA_KEY_Z) == COCOA_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_REDO, COCOA_GetKeyState(win, COCOA_KEY_R) == COCOA_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, COCOA_GetKeyState(win, COCOA_KEY_LEFT) == COCOA_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, COCOA_GetKeyState(win, COCOA_KEY_RIGHT) == COCOA_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_LINE_START, COCOA_GetKeyState(win, COCOA_KEY_B) == COCOA_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_LINE_END, COCOA_GetKeyState(win, COCOA_KEY_E) == COCOA_PRESS);
  }
  else
  {
    nk_input_key(ctx, NK_KEY_LEFT, COCOA_GetKeyState(win, COCOA_KEY_LEFT) == COCOA_PRESS);
    nk_input_key(ctx, NK_KEY_RIGHT, COCOA_GetKeyState(win, COCOA_KEY_RIGHT) == COCOA_PRESS);
    nk_input_key(ctx, NK_KEY_COPY, 0);
    nk_input_key(ctx, NK_KEY_PASTE, 0);
    nk_input_key(ctx, NK_KEY_CUT, 0);
    nk_input_key(ctx, NK_KEY_SHIFT, 0);
  }

  COCOA_GetMousePosition(win, &x, &y);
  nk_input_motion(ctx, (int)x, (int)y);
  nk_input_button(ctx, NK_BUTTON_LEFT, (int)x, (int)y,
                  COCOA_GetMouseButtonState(win, COCOA_MOUSE_BUTTON_LEFT) == COCOA_PRESS);
  nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y,
                  COCOA_GetMouseButtonState(win, COCOA_MOUSE_BUTTON_MIDDLE) == COCOA_PRESS);
  nk_input_button(ctx, NK_BUTTON_RIGHT, (int)x, (int)y,
                  COCOA_GetMouseButtonState(win, COCOA_MOUSE_BUTTON_RIGHT) == COCOA_PRESS);
  nk_input_button(ctx, NK_BUTTON_DOUBLE, (int)nk_cocoa.double_click_pos.x,
                  (int)nk_cocoa.double_click_pos.y, nk_cocoa.is_double_click_down);
  nk_input_scroll(ctx, nk_cocoa.scroll);
  nk_input_end(&nk_cocoa.ctx);
  nk_cocoa.text_len = 0;
  nk_cocoa.scroll = nk_vec2(0, 0);
}

NK_API
void nk_cocoa_shutdown(void)
{
  nk_font_atlas_clear(&nk_cocoa.atlas);
  nk_free(&nk_cocoa.ctx);
  nk_cocoa_device_destroy();
  memset(&nk_cocoa, 0, sizeof(nk_cocoa));
}

#endif

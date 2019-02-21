
#include "video_render.h"
#include "inslog.h"
#include "common.h"

struct target_ctx
{
	render_target target;
	GLuint frame_buff;
	GLuint texture;
};

void video_render::release()
{   
	for (auto it = target_.begin(); it != target_.end(); it++)
	{
		glDeleteFramebuffers(1, &it->second->frame_buff);
		glDeleteTextures(1, &it->second->texture);
	}
}

int video_render::setup(render_target& target)
{
	int ret;
	ret = setup_egl(target);
	RETURN_IF_NOT_OK(ret);

	ret = egl_.make_current(target.index);
	RETURN_IF_NOT_OK(ret);

	ret = setup_gles();
	RETURN_IF_NOT_OK(ret);

	do_add_render_target(target);

	LOGINFO("video render target index:%d w:%d h:%d", target.index, target.width, target.height);

	return INS_OK;
}

int video_render::setup_egl(render_target& target)
{
	if (target.index == TARGET_INDEX_SCN)
	{
		int ret = egl_.setup_screen(TARGET_INDEX_SCN,target.width,target.height);
		RETURN_IF_NOT_OK(ret);
	}
	else
	{
		int ret = egl_.setup_window(target.index, target.native_wnd);
		RETURN_IF_NOT_OK(ret);
	}

	return INS_OK;
}

int video_render::setup_gles()
{
	int ret;
	ret = create_master_program();
	RETURN_IF_NOT_OK(ret);

	ret = create_aux_program();
	RETURN_IF_NOT_OK(ret);

	gen_texture();

	glDisable(GL_DEPTH_TEST);
	glUseProgram(program_[0]);
	prepare_vao();

	return INS_OK;
}

int video_render::add_render_target(render_target& target)
{
	LOGINFO("video render add target index:%d w:%d h:%d", target.index, target.width, target.height);
	
	int ret = setup_egl(target);
	RETURN_IF_NOT_OK(ret);

	do_add_render_target(target);

	return INS_OK;
}

void video_render::do_add_render_target(render_target& target)
{
	GLuint texture;
	GLuint frame_buff;
	glGenTextures(1, &texture);
	glGenFramebuffers(1, &frame_buff);

	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, target.width, target.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buff);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	auto contex = std::make_shared<target_ctx>();
	contex->target = target;
	contex->frame_buff = frame_buff;
	contex->texture = texture;
	target_.insert(std::make_pair(target.width*target.height, contex));
}

void video_render::delete_render_target(int index)
{
	LOGINFO("video render delete render target:%d", index);

	for (auto it = target_.begin(); it != target_.end(); it++)
	{
		if (it->second->target.index == index)
		{
			glDeleteTextures(1, &it->second->texture);
			glDeleteFramebuffers(1,  &it->second->frame_buff);
			egl_.release_surface(index);
			target_.erase(it);
			break;
		}
	}
}

void video_render::draw(long long pts, float* mat)
{	
	if (target_.size() == 1)
	{
		auto target = target_.begin()->second->target;

		egl_.make_current(target.index);
		glUseProgram(program_[0]);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		draw_master_vao(target.width, target.height, mat);
		
		glFlush();
		glFinish();
		egl_.presentation_time(pts);//nsecs
		egl_.swap_buffers();
	}
	else if (target_.size() >= 2)
	{
		auto ctx = target_.begin()->second;
		//LOGINFO("max render w:%d h:%d", ctx->target.width, ctx->target.height);

		//1:render to framebuffer
		egl_.make_current(ctx->target.index);
		glUseProgram(program_[0]);
		glBindFramebuffer(GL_FRAMEBUFFER, ctx->frame_buff);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		draw_master_vao(ctx->target.width, ctx->target.height, mat);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		glFlush();
		glFinish();

		//2:render to first target
		glUseProgram(program_[1]);
		glViewport(0, 0, ctx->target.width, ctx->target.height);
		draw_aux_vao(ctx->texture);

		glFlush();
		glFinish();
		egl_.presentation_time(pts);//nsecs
		egl_.swap_buffers();

		//3:render to other target
		for (auto it = target_.begin(); it != target_.end(); it++)
		{	
			if (it == target_.begin()) continue;

			auto target = it->second->target;
			egl_.make_current(target.index);
			glClearColor(0, 0, 0, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glViewport(0, 0, target.width, target.height);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

			glFlush();
			glFinish();
			egl_.presentation_time(pts);//nsecs
			egl_.swap_buffers();
		}
	}
	else
	{
		LOGINFO("draw but no window started");
	}
}


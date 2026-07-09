#ifndef __ZXTUNE_PLAYER_H__
#define __ZXTUNE_PLAYER_H__

#include <tools/cycle_buffer.h>
#include <make_ptr.h>
#include <parameters/container.h>
#include <core/service.h>
#include <module/holder.h>
#include <module/players/pipeline.h>
#include <sound/receiver.h>

#pragma once

namespace ZXTune
{

class BufferRender : public Sound::Receiver
{
public:
	typedef std::shared_ptr<BufferRender> Ptr;

	BufferRender()
		: Buffer(32768)
		, DoneSamples()
	{
	}

	void ApplyData(Sound::Chunk data) override
	{
		Buffer.Put(data.begin(), data.size());
	}

	void Flush() override
	{
	}

	std::size_t GetCurrentSample() const
	{
		return DoneSamples;
	}

	std::size_t GetSamples(std::size_t count, Sound::Sample* target)
	{
		const Sound::Sample* part1 = 0;
		std::size_t part1Size = 0;
		const Sound::Sample* part2 = 0;
		std::size_t part2Size = 0;
		if(const std::size_t toGet = Buffer.Peek(count, part1, part1Size, part2, part2Size))
		{
			std::memcpy(target, part1, part1Size * sizeof(*part1));
			if(part2)
			{
				std::memcpy(target + part1Size, part2, part2Size * sizeof(*part2));
			}
			Buffer.Consume(toGet);
			DoneSamples += toGet;
			return toGet;
		}
		return 0;
	}

	void Reset()
	{
		Buffer.Reset();
		DoneSamples = 0;
	}
private:
	CycleBuffer<Sound::Sample> Buffer;
	std::size_t DoneSamples;
};

class PlayerWrapper
{
public:
	typedef std::shared_ptr<PlayerWrapper> Ptr;

	PlayerWrapper(Parameters::Container::Ptr params, Module::Renderer::Ptr renderer, BufferRender::Ptr buffer)
		: Params(params)
		, Renderer(renderer)
		, Buffer(buffer)
	{
	}

	std::size_t RenderSound(Sound::Sample* target, std::size_t samples)
	{
		std::size_t result = 0;
		while(samples)
		{
			const std::size_t got = Buffer->GetSamples(samples, target);
			target += got;
			samples -= got;
			result += got;
			if(!samples)
				break;
			auto chunk = Renderer->Render();
            if(chunk.empty())
				break;
            Buffer->ApplyData(std::move(chunk));
		}
		return result;
	}

	void Seek(double p_seconds)
	{
		Renderer->SetPosition(Time::AtMillisecond(p_seconds*1000.0));
	}

	void Reset()
	{
		Renderer->Reset();
		Buffer->Reset();
	}

	Parameters::Container::Ptr GetParameters() const
	{
		return Params;
	}

	static Ptr Create(const Module::Holder& holder)
	{
		const Parameters::Container::Ptr params = Parameters::Container::Create();
		//copy initial properties
		holder.GetModuleProperties()->Process(*params);
		const BufferRender::Ptr buffer = MakePtr<BufferRender>();
		const Module::Renderer::Ptr renderer = Module::CreatePipelinedRenderer(holder, params);
		return MakePtr<PlayerWrapper>(params, renderer, buffer);
	}
private:
	const Parameters::Container::Ptr Params;
	const Module::Renderer::Ptr Renderer;
	const BufferRender::Ptr Buffer;
};

static const auto& GetService()
{
	static const auto service = ZXTune::Service::Create(Parameters::Container::Create());
	return *service;
}

}
//namespace ZXTune

#endif//__ZXTUNE_PLAYER_H__

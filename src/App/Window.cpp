#include "Window.h"

#include <QMouseEvent>
#include <QLabel>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QVBoxLayout>
#include <QScreen>
#include <QDateTime>
#include <QSlider>

#include <array>
#include <chrono>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tinygltf/tiny_gltf.h>

Window::Window() noexcept
{
	const auto formatFPS = [](const auto value) {
		return QString("FPS: %1").arg(QString::number(value));
	};

	auto fps = new QLabel(formatFPS(0), this);
	fps->setStyleSheet("QLabel { color : white; }");

	
	const float SLIDER_MULT = 100;

	auto ambient_slider = new QSlider(Qt::Horizontal);
	ambient_slider->setRange(0, 10 * SLIDER_MULT);
	ambient_slider->setValue(0.5f * SLIDER_MULT);
	connect(ambient_slider, &QSlider::valueChanged, this, [this, SLIDER_MULT](float value) { ambientStrength_ = value / SLIDER_MULT; });

	auto ambient_label = new QLabel("Ambient: 0", this);
	ambient_label->setStyleSheet("QLabel { color : white; }");


	auto diffuse_slider = new QSlider(Qt::Horizontal);
	diffuse_slider->setRange(0, 10 * SLIDER_MULT);
	diffuse_slider->setValue(1 * SLIDER_MULT);
	connect(diffuse_slider, &QSlider::valueChanged, this, [this, SLIDER_MULT](float value) { diffuseReflection_ = value / SLIDER_MULT; });

	auto diffuse_label = new QLabel("Diffuse: 0", this);
	diffuse_label->setStyleSheet("QLabel { color : white; }");

	
	auto light1_slider = new QSlider(Qt::Horizontal);
	light1_slider->setRange(0, 1 * SLIDER_MULT);
	light1_slider->setValue(0.9f * SLIDER_MULT);
	connect(light1_slider, &QSlider::valueChanged, this, [this, SLIDER_MULT](float value) { Light1Param_ = value / SLIDER_MULT; });

	auto light1_label = new QLabel("Light1: 0", this);
	light1_label->setStyleSheet("QLabel { color : white; }");

	
	auto light2_slider = new QSlider(Qt::Horizontal);
	light2_slider->setRange(0, 1 * SLIDER_MULT);
	light2_slider->setValue(1 * SLIDER_MULT);
	connect(light2_slider, &QSlider::valueChanged, this, [this, SLIDER_MULT](float value) { Light2Param_ = value / SLIDER_MULT; });

	auto light2_label = new QLabel("Light2: 0", this);
	light2_label->setStyleSheet("QLabel { color : white; }");


	auto shininess_slider = new QSlider(Qt::Horizontal);
	shininess_slider->setRange(0, 100 * SLIDER_MULT);
	shininess_slider->setValue(30 * SLIDER_MULT);
	connect(shininess_slider, &QSlider::valueChanged, this, [this, SLIDER_MULT](float value) { shininess_ = value / SLIDER_MULT; });

	auto shininess_label = new QLabel("Shininess: 0", this);
	shininess_label->setStyleSheet("QLabel { color : white; }");

	
	auto specular_slider = new QSlider(Qt::Horizontal);
	specular_slider->setRange(0, 10 * SLIDER_MULT);
	specular_slider->setValue(1 * SLIDER_MULT);
	connect(specular_slider, &QSlider::valueChanged, this, [this, SLIDER_MULT](float value) { specular_ = value / SLIDER_MULT; });

	auto specular_label = new QLabel("Specular: 0", this);
	specular_label->setStyleSheet("QLabel { color : white; }");

	
	auto morph_slider = new QSlider(Qt::Horizontal);
	morph_slider->setRange(0, 10 * SLIDER_MULT);
	morph_slider->setValue(0.2f * SLIDER_MULT);
	connect(morph_slider, &QSlider::valueChanged, this, [this, SLIDER_MULT](float value) { morphSpeed_ = value / SLIDER_MULT; });

	auto morph_label = new QLabel("Morph: 0", this);
	morph_label->setStyleSheet("QLabel { color : white; }");


	auto layout = new QVBoxLayout();
	layout->addWidget(fps, 1);
	layout->addWidget(ambient_label);
	layout->addWidget(ambient_slider);
	layout->addWidget(diffuse_label);
	layout->addWidget(diffuse_slider);
	layout->addWidget(light1_slider);
	layout->addWidget(light1_label);
	layout->addWidget(light2_slider);
	layout->addWidget(light2_label);
	layout->addWidget(diffuse_slider);
	layout->addWidget(shininess_slider);
	layout->addWidget(shininess_label);
	layout->addWidget(specular_slider);
	layout->addWidget(specular_label);
	layout->addWidget(morph_slider);
	layout->addWidget(morph_label);

	setLayout(layout);

	timer_.start();

	connect(this, &Window::updateUI, [=] {
		fps->setText(formatFPS(ui_.fps));
		ambient_label->setText(QString("Ambient: %1").arg(ambientStrength_));
		diffuse_label->setText(QString("Diffuse: %1").arg(diffuseReflection_));
		light1_label->setText(QString("Light1: %1").arg(Light1Param_));
		light2_label->setText(QString("Light2: %1").arg(Light2Param_));
		shininess_label->setText(QString("Shininess: %1").arg(shininess_));
		specular_label->setText(QString("Specular: %1").arg(specular_));
		morph_label->setText(QString("Morph: %1").arg(morphSpeed_));
	});
}

Window::~Window()
{
	{
		// Free resources with context bounded.
		const auto guard = bindContext();
		for (auto & p: primitives_data)
		{
			p.tex.reset();
			p.normals.reset();
		}
		program_.reset();
	}
}

size_t read_inds(const tinygltf::Primitive & primitive, const tinygltf::Model & model, size_t model_vertexes_size, std::vector<GLuint> & model_indices)
{
	assert(primitive.indices >= 0);
	const auto & accessor_ind = model.accessors[primitive.indices];
	const auto & bufferView_ind = model.bufferViews[accessor_ind.bufferView];
	const auto & buffer_ind = model.buffers[bufferView_ind.buffer];

	size_t indexCount = accessor_ind.count;
	size_t byteOffset = accessor_ind.byteOffset + bufferView_ind.byteOffset;
	assert(accessor_ind.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT || accessor_ind.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT);
	assert(accessor_ind.type == TINYGLTF_TYPE_SCALAR);
	if (accessor_ind.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
	{
		std::vector<GLuint> uintIndices(indexCount);
		memcpy(uintIndices.data(), &buffer_ind.data[byteOffset], indexCount * sizeof(GLuint));
		model_indices.reserve(model_vertexes_size + uintIndices.size());
		for (auto & i: uintIndices)
		{
			model_indices.push_back(i + model_vertexes_size);
		}
	}
	else if (accessor_ind.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
	{
		std::vector<GLushort> shortIndices(indexCount);
		memcpy(shortIndices.data(), &buffer_ind.data[byteOffset], indexCount * sizeof(GLushort));
		model_indices.reserve(model_vertexes_size + shortIndices.size());
		for (auto i: shortIndices)
		{
			model_indices.push_back(i + model_vertexes_size);
		}
	}

	return indexCount;
}

std::pair<const void *, size_t> read_attribute(const tinygltf::Primitive & primitive, const tinygltf::Model & model, const std::string & key)
{
	assert(primitive.attributes.contains(key));
	const auto & accessor = model.accessors[primitive.attributes.at(key)];
	assert(!accessor.sparse.isSparse);
	const auto & bufferView = model.bufferViews[accessor.bufferView];
	const auto & buffer = model.buffers[bufferView.buffer];
	return {static_cast<const void *>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]), accessor.count};
}

QMatrix4x4 read_verts(const tinygltf::Primitive & primitive, const tinygltf::Node & node, const tinygltf::Model & model, std::vector<Vertex> & model_vertices, QMatrix4x4 parent_transform)
{
	QMatrix4x4 transform;
	transform.setToIdentity();

	if (node.translation.size() == 3)
	{
		transform.translate(node.translation[0], node.translation[1], node.translation[2]);
	}
	if (node.rotation.size() == 4)
	{
		QQuaternion q(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
		transform.rotate(q);
	}
	if (node.scale.size() == 3)
	{
		transform.scale(node.scale[0], node.scale[1], node.scale[2]);
	}

	transform = parent_transform * transform;

	{
		assert(primitive.attributes.contains("POSITION"));
		[[maybe_unused]] const auto & accessor = model.accessors[primitive.attributes.at("POSITION")];
		assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
		assert(accessor.type == TINYGLTF_TYPE_VEC3);
	}
	auto position_data = read_attribute(primitive, model, "POSITION");
	auto positions = reinterpret_cast<const QVector3D *>(position_data.first);

	{
		assert(primitive.attributes.contains("TEXCOORD_0"));
		[[maybe_unused]] const auto & accessor_tex = model.accessors[primitive.attributes.at("TEXCOORD_0")];
		assert(accessor_tex.type == TINYGLTF_TYPE_VEC2);
		assert(accessor_tex.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
	}
	auto texcoords_data = read_attribute(primitive, model, "TEXCOORD_0");
	const QVector2D * tex_coords = reinterpret_cast<const QVector2D *>(texcoords_data.first);

	{
		assert(primitive.attributes.contains("NORMAL"));
		[[maybe_unused]] const auto & accessor_tex = model.accessors[primitive.attributes.at("NORMAL")];
		assert(accessor_tex.type == TINYGLTF_TYPE_VEC3);
		assert(accessor_tex.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
	}
	auto normals_data = read_attribute(primitive, model, "NORMAL");
	const QVector3D * normals = reinterpret_cast<const QVector3D *>(normals_data.first);

	{
		assert(primitive.attributes.contains("TANGENT"));
		[[maybe_unused]] const auto & accessor_tan = model.accessors[primitive.attributes.at("TANGENT")];
		assert(accessor_tan.type == TINYGLTF_TYPE_VEC4);
		assert(accessor_tan.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);
	}
	auto tangents_data = read_attribute(primitive, model, "TANGENT");
	const QVector4D * tangents = reinterpret_cast<const QVector4D *>(tangents_data.first);

	assert(texcoords_data.second == position_data.second);
	assert(texcoords_data.second == normals_data.second);
	assert(texcoords_data.second == tangents_data.second);
	size_t verts_count = texcoords_data.second;

	std::vector<Vertex> local_verts;
	for (size_t i = 0; i < verts_count; i++)
	{
		QVector4D p(positions[i], 1.0f);
		p = transform * p;

		QVector3D bitangent = QVector3D::crossProduct(normals[i], tangents[i].toVector3D()) * tangents[i].w();
		local_verts.push_back({p.toVector3D(), normals[i], tex_coords[i], tangents[i].toVector3D(), bitangent});
	}

	model_vertices.insert(model_vertices.end(), local_verts.begin(), local_verts.end());

	return transform;
}

void Window::process_node(const tinygltf::Model & model, int32_t node_ind, std::vector<Vertex> & model_vertices, std::vector<GLuint> & model_indices, QMatrix4x4 parent_transform, int parent_texture)
{
	const auto & node = model.nodes[node_ind];
  
	if (node.mesh < 0)
		return;

	const auto & mesh = model.meshes[node.mesh];
	const auto & primitive = mesh.primitives[0];
	assert(primitive.mode == TINYGLTF_MODE_TRIANGLES);
	
	size_t model_vertexes_size = model_vertices.size();
	size_t indices_offset = model_indices.size();

	QMatrix4x4 transform = read_verts(primitive, node, model, model_vertices, parent_transform);
	size_t indexCount = read_inds(primitive, model, model_vertexes_size, model_indices);

	const auto & material = model.materials[primitive.material];
	const auto & texture = material.pbrMetallicRoughness.baseColorTexture.index > 0 ? model.textures[material.pbrMetallicRoughness.baseColorTexture.index] : model.textures[parent_texture];
	const auto & image = model.images[texture.source];
	assert(image.component == 4);
	
	const auto & normal_texture = model.textures[material.normalTexture.index];
	const auto normal_image = model.images[normal_texture.source];
	assert(normal_image.component == 4);
	
	auto create_texture = [](int width, int height, const unsigned char * image_data) {
		auto ans = std::make_unique<QOpenGLTexture>(QOpenGLTexture::Target2D);
		ans->setMinMagFilters(QOpenGLTexture::Linear, QOpenGLTexture::Linear);
		ans->setWrapMode(QOpenGLTexture::WrapMode::Repeat);
		ans->create();
		ans->setSize(width, height);
		ans->setFormat(QOpenGLTexture::TextureFormat::RGBA8_UNorm);
		ans->allocateStorage();
		ans->setData(QOpenGLTexture::RGBA, QOpenGLTexture::UInt8, image_data);
		ans->generateMipMaps();
		return ans;
	};

	Primitive p;

	p.normals = create_texture(normal_image.width, normal_image.height, normal_image.image.data());
	p.tex = create_texture(image.width, image.height, image.image.data());
	
	p.indices_offset = static_cast<int>(indices_offset);
	p.indices_size = static_cast<int>(indexCount);

	primitives_data.push_back(std::move(p));

	for (auto i: node.children)
	{
		assert(material.pbrMetallicRoughness.baseColorTexture.index > 0);
		process_node(model, i, model_vertices, model_indices, transform, material.pbrMetallicRoughness.baseColorTexture.index);
	}
}

void Window::onInit()
{
	// Configure shaders
	program_ = std::make_unique<QOpenGLShaderProgram>(this);
	program_->addShaderFromSourceFile(QOpenGLShader::Vertex, ":/Shaders/diffuse.vs");
	program_->addShaderFromSourceFile(QOpenGLShader::Fragment,
									  ":/Shaders/diffuse.fs");
	program_->link();

	// Create VAO object
	vao_.create();
	vao_.bind();

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	QFile file(":/Models/chess.glb");
	if (!file.open(QIODevice::ReadOnly))
	{
		printf("Failed to open resource:");
	}
	auto file_data = file.readAll();
	file.close();
	bool ret = loader.LoadBinaryFromMemory(&model, &err, &warn, (const unsigned char *)file_data.constData(), file_data.size());
	//bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, "Models/My.gltf");
	if (!warn.empty())
	{
		printf("Warn: %s\n", warn.c_str());
	}
	if (!err.empty())
	{
		printf("Err: %s\n", err.c_str());
	}
	if (!ret)
	{
		printf("Failed to parse glTF\n");
	}

	// Create VBO
	vbo_.create();
	vbo_.bind();
	vbo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
	
	const auto & scene = model.scenes[model.defaultScene];

	std::vector<GLuint> model_indices;
	std::vector<Vertex> model_vertices;

	for (auto node_ind: scene.nodes)
	{
		process_node(model, node_ind, model_vertices, model_indices);
	}

	vbo_.allocate(model_vertices.data(), static_cast<int>(model_vertices.size() * sizeof(Vertex)));

	// Create IBO
	ibo_.create();
	ibo_.bind();
	ibo_.setUsagePattern(QOpenGLBuffer::StaticDraw);
	ibo_.allocate(model_indices.data(), static_cast<int>(model_indices.size() * sizeof(GLuint)));

	// Bind attributes
	program_->bind();

	program_->enableAttributeArray(0);
	program_->setAttributeBuffer(0, GL_FLOAT, offsetof(Vertex, pos), 3, sizeof(Vertex));

	program_->enableAttributeArray(1);
	program_->setAttributeBuffer(1, GL_FLOAT, offsetof(Vertex, normal), 3, sizeof(Vertex));

	program_->enableAttributeArray(2);
	program_->setAttributeBuffer(2, GL_FLOAT, offsetof(Vertex, tex), 2, sizeof(Vertex));

	program_->enableAttributeArray(3);
	program_->setAttributeBuffer(3, GL_FLOAT, offsetof(Vertex, tangent), 3, sizeof(Vertex));

	program_->enableAttributeArray(4);
	program_->setAttributeBuffer(4, GL_FLOAT, offsetof(Vertex, bitangent), 3, sizeof(Vertex));

	mvpUniform_ = program_->uniformLocation("mvp");
	modelUniform_ = program_->uniformLocation("model");
	viewUniform_ = program_->uniformLocation("view");
	ambientStrengthUniform_ = program_->uniformLocation("ambientStrength");
	diffuseReflectionUniform_ = program_->uniformLocation("diffuseReflection");
	Light1ParamUniform_ = program_->uniformLocation("Light1Param");
	Light2ParamUniform_ = program_->uniformLocation("Light2Param");
	shininessUniform_ = program_->uniformLocation("shininess");
	specularUniform_ = program_->uniformLocation("specularStrength");
	cameraPosUniform_ = program_->uniformLocation("cameraPos");
	timeValueUniform_ = program_->uniformLocation("timeValue");
	morphSpeedUniform_ = program_->uniformLocation("morphSpeed");

	// Release all
	program_->release();

	vao_.release();

	ibo_.release();
	vbo_.release();

	// Еnable depth test and face culling
	glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LESS);
	//glEnable(GL_CULL_FACE);

	// Clear all FBO buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::updateMoving() {
	const float cameraSpeed = 0.01f;

	QMatrix4x4 A;
	A.rotate(cameraRotationX, {1.0f, 0.0f, 0.0f});
	A.rotate(cameraRotationY, {0.0f, 1.0f, 0.0f});
	QVector3D forward = A.row(2).toVector3D();

	QVector3D right = QVector3D::crossProduct(forward, QVector3D(0, 1, 0)).normalized();
	QVector3D up = QVector3D(0, 1, 0);

	if (buttons_[0])
		cameraPosition += forward * cameraSpeed;

	if (buttons_[1])
		cameraPosition -= right * cameraSpeed;

	if (buttons_[2])
		cameraPosition -= forward * cameraSpeed;

	if (buttons_[3])
		cameraPosition += right * cameraSpeed;

	if (buttons_[4])
		cameraPosition += up * cameraSpeed;

	if (buttons_[5])
		cameraPosition -= up * cameraSpeed;
}

void Window::onRender()
{
	updateMoving();

	const auto guard = captureMetrics();

	// Clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Calculate MVP matrix
	model_.setToIdentity();

	view_.setToIdentity();
	view_.rotate(cameraRotationX, {1.0f, 0.0f, 0.0f});
	view_.rotate(cameraRotationY, {0.0f, 1.0f, 0.0f});
	view_.translate(cameraPosition);
	const auto mvp = projection_ * view_ * model_;

	// Bind VAO and shader program
	program_->bind();
	vao_.bind();

	// Update uniform value
	program_->setUniformValue(mvpUniform_, mvp);
	program_->setUniformValue(modelUniform_, model_);
	program_->setUniformValue(viewUniform_, view_);
	program_->setUniformValue(ambientStrengthUniform_, ambientStrength_);
	program_->setUniformValue(diffuseReflectionUniform_, diffuseReflection_);
	program_->setUniformValue(Light1ParamUniform_, Light1Param_);
	program_->setUniformValue(Light2ParamUniform_, Light2Param_);
	program_->setUniformValue(shininessUniform_, shininess_);
	program_->setUniformValue(specularUniform_, specular_);
	program_->setUniformValue(cameraPosUniform_, cameraPosition);
	program_->setUniformValue(morphSpeedUniform_, morphSpeed_);
	
	static auto start_time = std::chrono::high_resolution_clock::now();
	float timeValue = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
	program_->setUniformValue(timeValueUniform_, timeValue);

	for (const auto & primitive: primitives_data)
	{
		glActiveTexture(GL_TEXTURE1);
		primitive.normals->bind();
		glActiveTexture(GL_TEXTURE0);
		primitive.tex->bind();
		program_->setUniformValue(program_->uniformLocation("tex_2d"), 0);
		program_->setUniformValue(program_->uniformLocation("normal_tex"), 1);

		glDrawElements(GL_TRIANGLES, primitive.indices_size, GL_UNSIGNED_INT, (void *)(primitive.indices_offset * sizeof(GLuint)));

		primitive.tex->release();
		primitive.normals->release();
	}

	// Release VAO and shader program
	vao_.release();
	program_->release();

	++frameCount_;

	// Request redraw if animated
	if (animated_)
	{
		update();
	}
}

void Window::onResize(const size_t width, const size_t height)
{
	// Configure viewport
	glViewport(0, 0, static_cast<GLint>(width), static_cast<GLint>(height));

	// Configure matrix
	const auto aspect = static_cast<float>(width) / static_cast<float>(height);
	const auto zNear = 0.1f;
	const auto zFar = 100.0f;
	const auto fov = 60.0f;
	projection_.setToIdentity();
	projection_.perspective(fov, aspect, zNear, zFar);
}

void Window::mouseMoveEvent(QMouseEvent * e)
{
	if (dragging_)
	{
		QPoint delta = e->pos() - lastMousePos_;
		lastMousePos_ = e->pos();

		cameraRotationX += delta.y() * 0.1f;
		cameraRotationY += delta.x() * 0.05f;
	}
}

void Window::mousePressEvent(QMouseEvent * e)
{
	dragging_ = true;
	lastMousePos_ = e->pos();
}

void Window::mouseReleaseEvent(QMouseEvent *)
{
	dragging_ = false;
}

void Window::keyPressEvent(QKeyEvent * event)
{
	switch (event->key())
	{
		case Qt::Key_W:
			buttons_[0] = true;
			break;
		case Qt::Key_A:
			buttons_[1] = true;
			break;
		case Qt::Key_S:
			buttons_[2] = true;
			break;
		case Qt::Key_D:
			buttons_[3] = true;
			break;
		case Qt::Key_Control:
			buttons_[4] = true;
			break;
		case Qt::Key_Space:
			buttons_[5] = true;
			break;
		default:
			break;
	}
}

void Window::keyReleaseEvent(QKeyEvent * event)
{
	switch (event->key())
	{
		case Qt::Key_W:
			buttons_[0] = false;
			break;
		case Qt::Key_A:
			buttons_[1] = false;
			break;
		case Qt::Key_S:
			buttons_[2] = false;
			break;
		case Qt::Key_D:
			buttons_[3] = false;
			break;
		case Qt::Key_Control:
			buttons_[4] = false;
			break;
		case Qt::Key_Space:
			buttons_[5] = false;
			break;
		default:
			break;
	}
}

Window::PerfomanceMetricsGuard::PerfomanceMetricsGuard(std::function<void()> callback)
	: callback_{ std::move(callback) }
{
}

Window::PerfomanceMetricsGuard::~PerfomanceMetricsGuard()
{
	if (callback_)
	{
		callback_();
	}
}

auto Window::captureMetrics() -> PerfomanceMetricsGuard
{
	return PerfomanceMetricsGuard{
		[&] {
			if (timer_.elapsed() >= 1000)
			{
				const auto elapsedSeconds = static_cast<float>(timer_.restart()) / 1000.0f;
				ui_.fps = static_cast<size_t>(std::round(frameCount_ / elapsedSeconds));
				frameCount_ = 0;
				emit updateUI();
			}
		}
	};
}
